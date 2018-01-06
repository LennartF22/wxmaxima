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
#include <wx/combo.h>
#include <wx/listctrl.h>

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
    Destroy();
    *m_doneptr = NULL;
    break;
  default:
    DeleteAllItems();
    int i = 0;
    for(wxArrayString::iterator it=m_completions.begin(); it != m_completions.end(); ++it)
      InsertItem(i, *it);

    SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
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
    int selection = GetNextItem(0,wxLIST_NEXT_ALL,
                                wxLIST_STATE_SELECTED);
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
    Destroy();
    *m_doneptr = NULL;
  }
  break;
  case WXK_LEFT:
  case WXK_ESCAPE:
    m_parent->GetParent()->Refresh();
    if (!m_editor->IsActive())
      m_editor->ActivateCursor();
    Destroy();
    *m_doneptr = NULL;
    break;
  case WXK_UP:
  {
    int selection = GetNextItem(0,wxLIST_NEXT_ALL,
                                wxLIST_STATE_SELECTED);
    if (selection > 0)
      SetItemState(selection - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    else
    {
      if (m_completions.GetCount() > 0)
        SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
    break;
  }
  case WXK_DOWN:
  {
    long selection = GetNextItem(0,wxLIST_NEXT_ALL,
                                 wxLIST_STATE_SELECTED);
    if (selection < 0) selection = 0;
    selection++;
    if (selection >= (long) m_completions.GetCount())
      selection--;
    if (m_completions.GetCount() > 0)
      SetItemState(selection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
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

    Destroy();
    *m_doneptr = NULL;
    break;
  }
  default:
    event.Skip();
  }
  m_parent->GetParent()->Refresh();
}

bool ContentAssistantPopup::Create(wxWindow* parent)
{
  bool retval = wxListView::Create(parent,1,wxDefaultPosition,wxDefaultSize,
                                   wxLC_ALIGN_LEFT |
                                   wxLC_REPORT |
                                   wxLC_NO_HEADER |
                                   wxLC_SINGLE_SEL |
                                   wxLC_SORT_ASCENDING);
  InsertColumn(0,wxEmptyString);
  UpdateResults();
  SetColumnWidth(0, wxLIST_AUTOSIZE);

  wxSize minSize;
  wxSize itemSpacing = GetItemSpacing();
  wxSize optimumSize = wxSize(-1,itemSpacing.y);
  for (size_t i = 0; i < m_completions.GetCount(); i++)
  {
    optimumSize.y += itemSpacing.y;
    wxRect itemRect;
    if(GetItemRect(i, itemRect))
    {
      if(optimumSize.x < itemRect.GetWidth())
        optimumSize.x = itemRect.GetWidth();
      
      optimumSize.y += itemRect.GetHeight();
    }
  }
  std::cerr<<"width="<<optimumSize.x<<"\n";
  minSize = optimumSize;
  if (minSize.y > 400) minSize.y = 400;
  SetMinSize(minSize);
  Layout();
  //SetOptimumSize(optimumSize);
  return retval;
}

void ContentAssistantPopup::OnClick(wxMouseEvent &event)
{
  if (GetItemCount() <= 0)
    return;

  m_value = wxListView::GetFirstSelected();

  {
    if (m_value < 0) m_value = 0;
    m_editor->ReplaceSelection(
      m_editor->GetSelectionString(),
      m_completions[m_value]
      );
    m_parent->GetParent()->Refresh();
    if (!m_editor->IsActive())
      m_editor->ActivateCursor();
    Destroy();
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
  wxWindow *parent, EditorCell *editor, AutoComplete *autocomplete,
  AutoComplete::autoCompletionType type,
  ContentAssistantPopup **doneptr): wxListView(), wxComboPopup()
{
  m_parent = parent;
  m_doneptr = doneptr;
  m_autocomplete = autocomplete;
  m_editor = editor;
  m_type = type;
  m_length = 0;
  
//  Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
//          wxCommandEventHandler(ContentAssistantPopup::OnClick),
//          NULL, this);
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
    int selection = GetNextItem(0,wxLIST_NEXT_ALL,
                                wxLIST_STATE_SELECTED);

    // The current key is no more part of the current command
    //
    // => Add the current selection to the worksheet and handle this keypress normally.
    if (selection < 0)
      selection = 0;
        
    m_editor->ReplaceSelection(
      m_editor->GetSelectionString(),
      m_completions[selection]
      );
    m_parent->GetParent()->Refresh();
    if (!m_editor->IsActive())
      m_editor->ActivateCursor();
    Destroy();
    *m_doneptr = NULL;
        
    // Tell MathCtrl to handle this key event the normal way.
    wxKeyEvent *keyEvent = new wxKeyEvent(event);
    m_parent->GetEventHandler()->QueueEvent(keyEvent);
    return;
  }
}

wxBEGIN_EVENT_TABLE(ContentAssistantPopup, wxListView)
EVT_MOTION(ContentAssistantPopup::OnMouseMove)
EVT_LEFT_UP(ContentAssistantPopup::OnClick)
EVT_CLOSE(ContentAssistantPopup::OnClose)
wxEND_EVENT_TABLE()


