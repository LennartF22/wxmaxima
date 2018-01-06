// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2009-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//  Copyright (C) 2015-2017 Gunter Königsmann     <wxMaxima@physikbuch.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

/*! \file
  This file defines the class ContentAssistantPopup.

  The content assistant offers more functionality than AutocompletePopup but 
  only works on systems that allow popups to handle key presses.
*/

#include "ContentAssistantPopup.h"
#include "Dirstructure.h"
#include "wxMaximaFrame.h"

#include <wx/textfile.h>

void ContentAssistantPopup::UpdateResults()
{
  wxString partial = m_editor->GetSelectionString();
  m_completions = m_autocomplete->CompleteSymbol(partial, m_type);
  m_completions.Sort();

  switch (m_completions.GetCount())
  {
    case 1:
      m_editor->ReplaceSelection(
              m_editor->GetSelectionString(),
              m_completions[0]
      );
    case 0:
      m_editor->ClearSelection();
      m_parent->GetParent()->Refresh();
      if (!m_editor->IsActive())
        m_editor->ActivateCursor();
      Dismiss();
      *m_doneptr = NULL;
      break;
    default:
      Clear();
      for(wxArrayString::iterator it=m_completions.begin(); it != m_completions.end(); ++it)
        Append(*it);
//      Populate(m_completions);
      SetSelection(0);
  }
}

void ContentAssistantPopup::OnKeyDown(wxKeyEvent &event)
{
  switch (event.GetKeyCode())
  {
    case WXK_TAB:
      if (m_completions.GetCount() > 0)
      {
        wxChar ch;
        bool addChar = true;
        wxString word = m_editor->GetSelectionString();
        size_t index = word.Length();
        do
        {
          if (m_completions[0].Length() <= index)
            addChar = false;
          else
          {
            ch = m_completions[0][index];
            for (size_t i = 0; i < m_completions.GetCount(); i++)
              if ((m_completions[i].Length() < index + 1) || (m_completions[i][index] != ch))
                addChar = false;
          }

          if (addChar)
          {
            index++;
            word += ch;
          }
        } while (addChar);
        m_editor->ReplaceSelection(m_editor->GetSelectionString(), word, true);
      }
      break;
    case WXK_RETURN:
    case WXK_RIGHT:
    case WXK_NUMPAD_ENTER:
    {
      int selection = GetSelection();
      if (selection < 0)
        selection = 0;

      if (m_completions.GetCount() > 0)
        m_editor->ReplaceSelection(
                m_editor->GetSelectionString(),
                m_completions[selection]
        );
      m_parent->GetParent()->Refresh();
      if (!m_editor->IsActive())
        m_editor->ActivateCursor();
      Dismiss();
      *m_doneptr = NULL;
    }
      break;
    case WXK_LEFT:
    case WXK_ESCAPE:
      m_parent->GetParent()->Refresh();
      if (!m_editor->IsActive())
        m_editor->ActivateCursor();
      Dismiss();
      *m_doneptr = NULL;
      break;
    case WXK_UP:
    {
      int selection = GetSelection();
      if (selection > 0)
        SetSelection(selection - 1);
      else
      {
        if (m_completions.GetCount() > 0)
          SetSelection(0);
      }
      break;
    }
    case WXK_DOWN:
    {
      long selection = GetSelection();
      if (selection < 0) selection = 0;
      selection++;
      if (selection >= (long) m_completions.GetCount())
        selection--;
      if (m_completions.GetCount() > 0)
        SetSelection(selection);
      break;
    }
    case WXK_BACK:
    {
      wxString oldString = m_editor->GetSelectionString();
      if (oldString != wxEmptyString)
      {
        m_editor->ReplaceSelection(
                oldString,
                oldString.Left(oldString.Length() - 1),
                true
        );
        UpdateResults();
      }
      else
        m_parent->GetParent()->Refresh();
      if (!m_editor->IsActive())
        m_editor->ActivateCursor();

      Dismiss();
      *m_doneptr = NULL;
      break;
    }
    default:
      event.Skip();
  }
  m_parent->GetParent()->Refresh();
}

void ContentAssistantPopup::OnClick(wxCommandEvent &event)
{
  if (m_completions.GetCount() <= 0)
    return;
  {
    int selection = event.GetSelection();
    if (selection < 0) selection = 0;
    m_editor->ReplaceSelection(
            m_editor->GetSelectionString(),
            m_completions[selection]
    );
    m_parent->GetParent()->Refresh();
    if (!m_editor->IsActive())
      m_editor->ActivateCursor();
    Dismiss();
    *m_doneptr = NULL;
  }
}

void ContentAssistantPopup::OnDismiss()
{
  *m_doneptr = NULL;
}

void ContentAssistantPopup::OnClose(wxCloseEvent& WXUNUSED(event))
{
  *m_doneptr = NULL;
}

ContentAssistantPopup::~ContentAssistantPopup()
{
  *m_doneptr = NULL;
}

ContentAssistantPopup::ContentAssistantPopup(
        wxWindow *parent,
        EditorCell *editor,
        AutoComplete *autocomplete,
        AutoComplete::autoCompletionType type,
        ContentAssistantPopup **doneptr
) : wxVListBoxComboPopup()
{
  m_parent = parent;
  m_doneptr = doneptr;
  m_autocomplete = autocomplete;
  m_editor = editor;
  m_type = type;
  m_length = 0;
  
  Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
          wxCommandEventHandler(ContentAssistantPopup::OnClick),
          NULL, this);
}

void ContentAssistantPopup::OnChar(wxKeyEvent &event)
{
  wxChar key = event.GetUnicodeKey();
  if ((wxIsalpha(key)) || (key == wxT('_')) || (key == wxT('\"')) ||
      (
        (
          (m_type == AutoComplete::generalfile) ||
          (m_type == AutoComplete::loadfile) ||
          (m_type == AutoComplete::demofile)
          ) &&
        (key == wxT('/'))
        )
    )
  {
    wxString oldString = m_editor->GetSelectionString();
    m_editor->ReplaceSelection(
      oldString,
      oldString + wxString(key),
      true
      );
    UpdateResults();
    return;
  }
  else if (wxIsprint(key))
  {
    // The current key is no more part of the current command
    //
    // => Add the current selection to the worksheet and handle this keypress normally.
    int selection = GetSelection();
        if (selection < 0)
          selection = 0;
        
        m_editor->ReplaceSelection(
          m_editor->GetSelectionString(),
          m_completions[selection]
          );
        m_parent->GetParent()->Refresh();
        if (!m_editor->IsActive())
          m_editor->ActivateCursor();
        Dismiss();
        *m_doneptr = NULL;
        
        // Tell MathCtrl to handle this key event the normal way.
        wxKeyEvent *keyEvent = new wxKeyEvent(event);
        m_parent->GetEventHandler()->QueueEvent(keyEvent);
        return;
      }
}

BEGIN_EVENT_TABLE(ContentAssistantPopup, wxVListBoxComboPopup)
EVT_CLOSE(ContentAssistantPopup::OnClose)
END_EVENT_TABLE()
