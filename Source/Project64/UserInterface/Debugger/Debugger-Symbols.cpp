#include "stdafx.h"

#include "DebuggerUI.h"

#include <Common/path.h>
#include <stdio.h>

#include "Symbols.h"

const CSetValueDlg::ComboItem CDebugSymbols::ModalChangeTypeItems[] = {
    {"code", SYM_CODE},
    {"data", SYM_DATA},
    {"uint8", SYM_U8},
    {"int8", SYM_S8},
    {"uint16", SYM_U16},
    {"int16", SYM_S16},
    {"uint32", SYM_U32},
    {"int32", SYM_S32},
    {"uint64", SYM_U64},
    {"int64", SYM_S64},
    {"float", SYM_FLOAT},
    {"double", SYM_DOUBLE},
    {"v2", SYM_VECTOR2},
    {"v3", SYM_VECTOR3},
    {"v4", SYM_VECTOR4},
    { nullptr, 0 }
};

CDebugSymbols::CDebugSymbols(CDebuggerUI * debugger) :
    CDebugDialog<CDebugSymbols>(debugger),
    m_SymbolTable(debugger->SymbolTable()),
    m_SymbolCacheStartIndex(0),
    m_bFiltering(false)
{
    memset(m_FilterText, 0, sizeof(m_FilterText));
}

LRESULT CDebugSymbols::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_SymbolsPos);

    m_SymbolsListView.Attach(GetDlgItem(IDC_SYMBOLS_LIST));
    m_SymbolsListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_SymbolsListView.AddColumn(L"Address", 0);
    m_SymbolsListView.AddColumn(L"Type", 1);
    m_SymbolsListView.AddColumn(L"Name", 2);
    m_SymbolsListView.AddColumn(L"Value", 3);
    m_SymbolsListView.AddColumn(L"Description", 4);

    m_SymbolsListView.SetColumnWidth(0, 70);
    m_SymbolsListView.SetColumnWidth(1, 40);
    m_SymbolsListView.SetColumnWidth(2, 120);
    m_SymbolsListView.SetColumnWidth(3, 100);
    m_SymbolsListView.SetColumnWidth(4, 120);

    ::SetWindowText(GetDlgItem(IDC_FILTER_EDIT), m_FilterText);

    Refresh();

    SetTimer(TIMER_ID_AUTO_REFRESH, 100, nullptr);

    LoadWindowPos();
    WindowCreated();
    return 0;
}

void CDebugSymbols::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugSymbols::OnDestroy(void)
{
    KillTimer(TIMER_ID_AUTO_REFRESH);
    m_SymbolsListView.Detach();
    return 0;
}

void CDebugSymbols::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID_AUTO_REFRESH)
    {
        Refresh();
    }
}

LRESULT CDebugSymbols::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_ADDSYMBOL_BTN:
        m_AddSymbolDlg.DoModal(m_Debugger);
        break;
    case IDC_REMOVESYMBOL_BTN:
    {
        int nItem = m_SymbolsListView.GetSelectedIndex();
        int id = -1;
        if (m_bFiltering)
        {
            id = m_FilteredSymbols[nItem].m_Id;
        }
        else
        {
            CSymbol symbol;
            m_SymbolTable->GetSymbolByIndex(nItem, &symbol);
            id = symbol.m_Id;
        }

        if (id != -1)
        {
            m_SymbolTable->RemoveSymbolById(id);
            m_SymbolTable->Save();
            Refresh();
        }
    }
    break;
    }
    return FALSE;
}

LRESULT CDebugSymbols::OnListDblClicked(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);

    if (pIA->iItem == -1)
    {
        return 0;
    }

    int id = GetListItemSymbolId(pIA->iItem);

    CSymbol symbol;
    if (!m_SymbolTable->GetSymbolById(id, &symbol))
    {
        return 0;
    }

    POINT mousePt;
    GetCursorPos(&mousePt);
    int nSelectedCol = ColumnHitTest(mousePt);

    switch (nSelectedCol)
    {
    case SymbolsListView_Col_Address:
        // Open it in memory viewer/commands viewer
        if (symbol.m_Type == SYM_CODE) // Code
        {
            m_Debugger->Debug_ShowCommandsLocation(symbol.m_Address, true);
        }
        else // Data/number
        {
            m_Debugger->Debug_ShowMemoryLocation(symbol.m_Address, true);
        }
        break;
    case SymbolsListView_Col_Type:
        if (m_SetValueDlg.DoModal("Change type", "New type:", symbol.m_Type, ModalChangeTypeItems))
        {
            ValueType t = (ValueType)m_SetValueDlg.GetEnteredData();

            // TODO: Is there a better way?
            m_SymbolTable->RemoveSymbolById(id);
            m_SymbolTable->AddSymbol(t, symbol.m_Address, symbol.m_Name, symbol.m_Description);
        }
        break;
    case SymbolsListView_Col_Name:
        if (m_SetValueDlg.DoModal("Set name", "New name:", symbol.m_Name))
        {
            m_SymbolTable->RemoveSymbolById(id);
            m_SymbolTable->AddSymbol(symbol.m_Type, symbol.m_Address, m_SetValueDlg.GetEnteredString().c_str(), symbol.m_Description);
        }
        break;
    case SymbolsListView_Col_Value:
        char szValue[256];
        const char * x;
        const char * y;
        m_SymbolTable->GetValueString(szValue, &symbol);
        if (m_SetValueDlg.DoModal("Change value", "New value:", szValue))
        {
            const std::string & EnteredString = m_SetValueDlg.GetEnteredString();

            switch (symbol.m_Type)
            {
            case SYM_U8:
                m_Debugger->DebugStore_VAddr<uint8_t>(symbol.m_Address, (uint8_t)atoi(EnteredString.c_str()));
                break;
            case SYM_U16:
                m_Debugger->DebugStore_VAddr<uint16_t>(symbol.m_Address, (uint16_t)atoi(EnteredString.c_str()));
                break;
            case SYM_U32:
                m_Debugger->DebugStore_VAddr<uint32_t>(symbol.m_Address, atoi(EnteredString.c_str()));
                break;
            case SYM_U64:
                m_Debugger->DebugStore_VAddr<uint64_t>(symbol.m_Address, atoll(EnteredString.c_str()));
                break;
            case SYM_S8:
                m_Debugger->DebugStore_VAddr<int8_t>(symbol.m_Address, (int8_t)atoi(EnteredString.c_str()));
                break;
            case SYM_S16:
                m_Debugger->DebugStore_VAddr<int16_t>(symbol.m_Address, (int16_t)atoi(EnteredString.c_str()));
                break;
            case SYM_S32:
                m_Debugger->DebugStore_VAddr<int>(symbol.m_Address, atoi(EnteredString.c_str()));
                break;
            case SYM_S64:
                m_Debugger->DebugStore_VAddr<int64_t>(symbol.m_Address, atoll(EnteredString.c_str()));
                break;
            case SYM_FLOAT:
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address, (float)atof(EnteredString.c_str()));
                break;
            case SYM_DOUBLE:
                m_Debugger->DebugStore_VAddr<double>(symbol.m_Address, atof(EnteredString.c_str()));
                break;
            case SYM_VECTOR2:
                x = EnteredString.c_str();
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address, (float)atof(szValue));

                x = x + (y - x) + 1;
                memcpy(szValue, x, strlen(x));
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + sizeof(float), (float)atof(szValue));
                break;
            case SYM_VECTOR3:
                x = EnteredString.c_str();
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address, (float)atof(szValue));

                x = x + (y - x) + 1;
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + sizeof(float), (float)atof(szValue));

                x = x + (y - x) + 1;
                memcpy(szValue, x, strlen(x));
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + (sizeof(float) * 2), (float)atof(szValue));
                break;
            case SYM_VECTOR4:
                x = EnteredString.c_str();
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address, (float)atof(szValue));

                x = x + (y - x) + 1;
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + sizeof(float), (float)atof(szValue));

                x = x + (y - x) + 1;
                y = strchr(x, ',');
                memcpy(szValue, x, y - x);
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + (sizeof(float) * 2), (float)atof(szValue));

                x = x + (y - x) + 1;
                memcpy(szValue, x, strlen(x));
                m_Debugger->DebugStore_VAddr<float>(symbol.m_Address + (sizeof(float) * 3), (float)atof(szValue));
                break;
            }
        }
        break;
    case SymbolsListView_Col_Description:
        if (m_SetValueDlg.DoModal("Set description", "New description:", symbol.m_Description))
        {
            m_SymbolTable->RemoveSymbolById(id);
            m_SymbolTable->AddSymbol(symbol.m_Type, symbol.m_Address, symbol.m_Name, m_SetValueDlg.GetEnteredString().c_str());
        }
        break;
    }

    m_SymbolTable->Save();
    Refresh();

    return 0;
}

LRESULT CDebugSymbols::OnListGetDispInfo(NMHDR * pNMHDR)
{
    NMLVDISPINFO * plvdi = (NMLVDISPINFO *)pNMHDR;

    int index = plvdi->item.iItem;

    if (index == -1)
    {
        return TRUE;
    }

    std::vector<SymbolCacheItem> & cache = m_bFiltering ? m_FilteredSymbols : m_SymbolCache;

    if (!m_bFiltering)
    {
        index -= m_SymbolCacheStartIndex;
    }

    switch (plvdi->item.iSubItem)
    {
    case SymbolsListView_Col_Address:
        plvdi->item.pszText = cache[index].m_Address;
        break;
    case SymbolsListView_Col_Type:
        plvdi->item.pszText = cache[index].m_Type;
        break;
    case SymbolsListView_Col_Name:
        plvdi->item.pszText = cache[index].m_Name;
        break;
    case SymbolsListView_Col_Value:
        plvdi->item.pszText = cache[index].m_Value;
        break;
    case SymbolsListView_Col_Description:
        plvdi->item.pszText = cache[index].m_Description;
        break;
    }

    return TRUE;
}

LRESULT CDebugSymbols::OnListCacheHint(NMHDR * pNMHDR)
{
    NMLVCACHEHINT * plvch = (NMLVCACHEHINT *)pNMHDR;

    if (m_bFiltering)
    {
        return 0;
    }

    m_SymbolCacheStartIndex = plvch->iFrom;
    m_SymbolCache.clear();

    for (int index = plvch->iFrom; index <= plvch->iTo; index++)
    {
        CSymbol symbol;
        if (!m_SymbolTable->GetSymbolByIndex(index, &symbol))
        {
            break;
        }

        SymbolCacheItem item(symbol, m_SymbolTable);
        m_SymbolCache.push_back(item);
    }

    return 0;
}

LRESULT CDebugSymbols::OnFilterChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    if (::GetWindowTextLength(GetDlgItem(IDC_FILTER_EDIT)) == 0)
    {
        m_bFiltering = false;
        Refresh();
        return FALSE;
    }

    m_bFiltering = true;
    ::GetWindowText(GetDlgItem(IDC_FILTER_EDIT), m_FilterText, sizeof(m_FilterText) / sizeof(wchar_t));

    UpdateFilteredSymbols();

    m_SymbolsListView.SetItemCount(m_FilteredSymbols.size());
    return FALSE;
}

int CDebugSymbols::ColumnHitTest(POINT & pt)
{
    int nHitCol = -1;
    RECT listRect;
    m_SymbolsListView.GetWindowRect(&listRect);

    int mouseX = pt.x - listRect.left;

    for (int nCol = 0, colX = 0; nCol < SymbolsListView_Num_Columns; nCol++)
    {
        int colWidth = m_SymbolsListView.GetColumnWidth(nCol);
        if (mouseX >= colX && mouseX <= colX + colWidth)
        {
            nHitCol = nCol;
            break;
        }
        colX += colWidth;
    }

    return nHitCol;
}

void CDebugSymbols::UpdateFilteredSymbols()
{
    m_FilteredSymbols.clear();

    CSymbol symbol;
    size_t index = 0;
    while (m_SymbolTable->GetSymbolByIndex(index++, &symbol))
    {
        std::wstring strName = stdstr(symbol.m_Name).ToUTF16();
        std::wstring strDesc = stdstr(symbol.m_Description).ToUTF16();

        if (strName.find(m_FilterText) != std::wstring::npos ||
            strDesc.find(m_FilterText) != std::wstring::npos)
        {
            SymbolCacheItem item(symbol, m_SymbolTable);
            m_FilteredSymbols.push_back(item);
        }
    }
}

void CDebugSymbols::Refresh()
{
    if (g_Settings->LoadStringVal(Game_GameName) != "")
    {
        stdstr windowTitle = stdstr_f("Symbols - %s", g_Settings->LoadStringVal(Game_GameName).c_str());
        SetWindowText(windowTitle.ToUTF16().c_str());
    }
    else
    {
        SetWindowText(L"Symbols");
    }

    if (m_SymbolsListView.m_hWnd == nullptr)
    {
        return;
    }

    int numSymbols = 0;

    if (m_bFiltering)
    {
        UpdateFilteredSymbols();
        numSymbols = m_FilteredSymbols.size();
    }
    else
    {
        numSymbols = m_SymbolTable->GetCount();
    }

    m_SymbolsListView.SetItemCountEx(numSymbols, LVSICF_NOSCROLL);
}

int CDebugSymbols::GetListItemSymbolId(int nItem)
{
    if (m_bFiltering)
    {
        if (nItem >= m_FilteredSymbols.size())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        return m_FilteredSymbols[nItem].m_Id;
    }

    CSymbol symbol;
    if (!m_SymbolTable->GetSymbolByIndex(nItem, &symbol))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    return symbol.m_Id;
}
