#pragma once

enum
{
    CIN_SPECIALKEY
};

typedef struct
{
    NMHDR nmh;
    int vkey;
} NMCISPECIALKEY;

class CEditConInput : public CWindowImpl<CEditConInput, CEdit>
{
public:
    CEditConInput()
    {
    }

    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & bHandled)
    {
        NMCISPECIALKEY nmsk;

        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        case VK_RETURN:
            nmsk = {{m_hWnd, (UINT_PTR)GetDlgCtrlID(), CIN_SPECIALKEY}, (int)wParam};
            SendMessage(GetParent(), WM_NOTIFY, nmsk.nmh.idFrom, (LPARAM)&nmsk);
            break;
        }

        bHandled = FALSE;

        return 0;
    }

    BOOL Attach(HWND hWndNew)
    {
        return SubclassWindow(hWndNew);
    }

    BEGIN_MSG_MAP_EX(CEditEval)
    {
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);
    }
    END_MSG_MAP()
};

class CEditConOutput : public CWindowImpl<CEditConOutput, CEdit>
{
private:
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
    {
        if (GetKeyState(VK_CONTROL) < 0)
        {
            if (wParam == 'A')
            {
                this->SetSelAll();
            }
        }
        return FALSE;
    }

public:
    BOOL Attach(HWND hWndNew)
    {
        return SubclassWindow(hWndNew);
    }

    BEGIN_MSG_MAP_EX(CEditEval)
    {
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);
    }
    END_MSG_MAP()
};
