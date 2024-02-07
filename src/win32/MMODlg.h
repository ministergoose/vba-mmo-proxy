#ifndef __MMO_DLG___
#define __MMO_DLG___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MMODlg : public CDialog
{
	DECLARE_DYNAMIC(MMODlg)

public:
	MMODlg(CWnd* pParent = nullptr); 
	virtual ~MMODlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GBA_MMO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	CString m_host;
	DWORD m_port;
	CString m_email;
	CString m_name;
	DWORD m_timeout;
	BOOL m_need_udp_init;
	CString m_cookie;
	afx_msg void OnBnClickedButtonCreate();
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnChangeServer();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnStnClickedPcDiscord();
};

#endif // !__MMO_DLG___
