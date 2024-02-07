#include "stdafx.h"
#include "MMODlg.h"
#include "../mmo/mytools.h"
#include "../mmo/udp.h"
#include "../mmo/account.h"
#include "afxdialogex.h"
#include "../mmo/gba_mmo.h"

IMPLEMENT_DYNAMIC(MMODlg, CDialog)

MMODlg::MMODlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_GBA_MMO, pParent)
	, m_host(_T("127.0.0.1"))
	, m_port(5465)
	, m_email(_T(""))
	, m_name(_T(""))
	, m_cookie("")
	, m_need_udp_init(TRUE)
	, m_timeout(500)
{
	

}

MMODlg::~MMODlg()
{
}

BOOL MMODlg::OnInitDialog() {
	CDialog::OnInitDialog();

	CString p;
	m_host = MMO_LoadParamStr("host", "127.0.0.1");
	GetDlgItem(IDC_SERVER_HOST)->SetWindowText(m_host);
	m_port = MMO_LoadParamInt("port", 5465);
	p.Format("%d", m_port);
	GetDlgItem(IDC_SERVER_PORT)->SetWindowText(p);
	m_timeout = MMO_LoadParamInt("timeout", 500);
	p.Format("%d", m_timeout);
	GetDlgItem(IDC_SERVER_TIMEOUT)->SetWindowText(p);
	m_email = MMO_LoadParamStr("email", "");
	GetDlgItem(IDC_EDIT_EMAIL)->SetWindowText(m_email);
	m_name = MMO_LoadParamStr("name", "");
	GetDlgItem(IDC_EDIT_USERNAME)->SetWindowText(m_name);
	m_cookie = MMO_LoadParamStr("cookie", "");

	UpdateData(FALSE);

	return TRUE;
}

void MMODlg::OnOK()
{
	CString buffer;
	GetDlgItem(IDC_SERVER_HOST)->GetWindowTextA(buffer);
	if(!buffer.IsEmpty())
		MMO_SaveParamStr("host", buffer);
	GetDlgItem(IDC_SERVER_PORT)->GetWindowTextA(buffer);
	if (!buffer.IsEmpty())
		MMO_SaveParamStr("port", buffer);
	GetDlgItem(IDC_SERVER_TIMEOUT)->GetWindowTextA(buffer);
	if (!buffer.IsEmpty())
		MMO_SaveParamStr("timeout", buffer);
	GetDlgItem(IDC_EDIT_EMAIL)->GetWindowTextA(buffer);
	if (!buffer.IsEmpty())
		MMO_SaveParamStr("email", buffer);
	GetDlgItem(IDC_EDIT_USERNAME)->GetWindowTextA(buffer);
	if (!buffer.IsEmpty())
		MMO_SaveParamStr("name", buffer);
	if(!m_cookie.IsEmpty())
		MMO_SaveParamStr("cookie", m_cookie);
	CDialog::OnOK();
}

void MMODlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString(pDX, IDC_SERVER_HOST, m_host);
	DDV_MaxChars(pDX, m_host, 255);
	DDX_Text(pDX, IDC_SERVER_PORT, m_port);
	DDV_MinMaxUInt(pDX, m_port, 0, 65535);
	DDX_Text(pDX, IDC_EDIT_EMAIL, m_email);
	DDV_MaxChars(pDX, m_email, 100);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_name);
	DDV_MaxChars(pDX, m_name, 100);
	DDX_Text(pDX, IDC_SERVER_TIMEOUT, m_timeout);
	DDV_MinMaxUInt(pDX, m_timeout, 50, 40000);
}


BEGIN_MESSAGE_MAP(MMODlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_CREATE, &MMODlg::OnBnClickedButtonCreate)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &MMODlg::OnBnClickedButtonLogin)
	ON_CBN_SELCHANGE(IDC_SERVER_HOST, &MMODlg::OnChangeServer)
	ON_CBN_EDITCHANGE(IDC_SERVER_HOST, &MMODlg::OnChangeServer)
	ON_EN_CHANGE(IDC_SERVER_TIMEOUT, &MMODlg::OnChangeServer)
	ON_EN_CHANGE(IDC_SERVER_PORT, &MMODlg::OnChangeServer)
	ON_WM_SETCURSOR()
	ON_STN_CLICKED(IDC_PC_DISCORD, &MMODlg::OnStnClickedPcDiscord)
END_MESSAGE_MAP()


void MMODlg::OnBnClickedButtonCreate()
{
	UpdateData(TRUE);

	if (m_name.IsEmpty() || m_email.IsEmpty()) {
		MessageBoxA("Please enter email and character name.", "Error!", MB_OK);
		return;
	}

	if (m_need_udp_init) {
		int res = MMO_UdpInit(m_host, (u_short)m_port, m_timeout);
		if (res != 0) {
			MessageBoxA("Couldn't create socket.", "Error!", MB_OK);
			return;
		}
		m_need_udp_init = FALSE;
	}

	char* cookie = MMO_CreateAccount(m_name, m_email);
	if (cookie != NULL && strlen(cookie) > 0) {
		MessageBoxA("Congratulations, your account has been successfully created.", "SUCCESS", MB_OK);
		m_cookie = cookie;
	}
	else {
		MessageBoxA("Couldn't create account.", "Error!", MB_OK);
	}
}


void MMODlg::OnBnClickedButtonLogin()
{
	if (m_cookie.IsEmpty()) {
		MessageBoxA("You must be registered for login.", "Error!", MB_OK);
		return;
	}

	if (m_need_udp_init) {
		UpdateData(TRUE);
		int res = MMO_UdpInit(m_host, (u_short)m_port, m_timeout);
		if (res != 0) {
			MessageBoxA("Couldn't create socket.", "Error!", MB_OK);
			return;
		}
		m_need_udp_init = FALSE;
	}

	if (MMO_Login(m_cookie)) {
		if (MMO_Start()) {
			OnOK();
		}
		else {
			MessageBoxA("Login failed.", "Error!", MB_OK);
		}
		
	}
	else {
		MessageBoxA("Login failed.", "Error!", MB_OK);
		return;
	}

}


void MMODlg::OnChangeServer()
{
	m_need_udp_init = TRUE;
}


BOOL MMODlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd->GetDlgCtrlID() == IDC_PC_DISCORD) {
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		return TRUE;
	}

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}


void MMODlg::OnStnClickedPcDiscord()
{
	ShellExecute(NULL, "open", "https://discord.gg/B3wJYnUswN", NULL, NULL, SW_SHOWNORMAL);
}
