/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  audit32.c

  Audit dialog

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <richedit.h>

#include "screenshot.h"
#include "win32ui.h"

#include <audit.h>
#include <unzip.h>

#include "resource.h"

#include "bitmask.h"
#include "options.h"
#include "..\windows\config.h"
#include "m32util.h"
#include "audit32.h"
#include "Properties.h"
#include "translate.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg);
static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void ProcessNextRom(void);
static void ProcessNextSample(void);
static void CLIB_DECL DetailsPrintf(const char *fmt, ...);
static const char * StatusString(int iStatus);

/***************************************************************************
    Internal variables
 ***************************************************************************/

#define SAMPLES_NOT_USED    3
#define MAX_TEXT		   2147483647

HWND hAudit;
static int rom_index;
static int roms_correct;
static int roms_incorrect;
static int sample_index;
static int samples_correct;
static int samples_incorrect;

static BOOL bPaused = FALSE;
static BOOL bCancel = FALSE;

/***************************************************************************
    External functions  
 ***************************************************************************/

void AuditDialog(HWND hParent)
{
	HMODULE hModule = NULL;
	rom_index         = 0;
	roms_correct      = 0;
	roms_incorrect    = 0;
	sample_index      = 0;
	samples_correct   = 0;
	samples_incorrect = 0;

	//RS use Riched32.dll
	// Riched32.dll doesn't work on Win9X
	// hModule = LoadLibraryA("Riched32.dll");
	hModule = LoadLibraryA("RICHED20.DLL");
	if( hModule )
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_AUDIT),hParent,AuditWindowProc);
		FreeLibrary( hModule );
		hModule = NULL;
	}
	else
	{
	    MessageBox(GetMainWindow(),_Unicode(_UI("Unable to Load Riched32.dll")),TEXT("Error"),
				   MB_OK | MB_ICONERROR);
	}
	
}

void InitGameAudit(int gameIndex)
{
	//mamep: DO NOT CALL cli_frontend_init(), or crash
	rom_index = gameIndex;
}

const char * GetAuditString(int audit_result)
{
	switch (audit_result)
	{
		case CORRECT:
		case BEST_AVAILABLE:
			return _UI("Yes");

		case INCORRECT:
		case NOTFOUND:
			return _UI("No");
			break;

		case UNKNOWN:
			return "?";

		default:
			dprintf("unknown audit value %i",audit_result);
			break;
	}

	return "?";
}

BOOL IsAuditResultKnown(int audit_result)
{
	return audit_result != UNKNOWN;
}

BOOL IsAuditResultYes(int audit_result)
{
	return audit_result == CORRECT || audit_result == BEST_AVAILABLE;
}

BOOL IsAuditResultNo(int audit_result)
{
	return audit_result == NOTFOUND || audit_result == INCORRECT;
}


/***************************************************************************
    Internal functions
 ***************************************************************************/

static void Mame32Output(void *param, const char *format, va_list argptr)
{
	char buffer[512];
	vsnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, argptr);
	DetailsPrintf("%s", buffer);
}

static int ProcessAuditResults(int game, audit_record *audit, int audit_records)
{
	output_callback prevcb;
	void *prevparam;
	int res;

	mame_set_output_channel(OUTPUT_CHANNEL_INFO, Mame32Output, NULL, &prevcb, &prevparam);
	res = audit_summary(game, audit_records, audit, TRUE);
	mame_set_output_channel(OUTPUT_CHANNEL_INFO, prevcb ? prevcb : mame_null_output_callback, prevparam, NULL, NULL);

	return res;
}

// Verifies the ROM set while calling SetRomAuditResults
int Mame32VerifyRomSet(int game)
{
	audit_record *audit;
	int audit_records;
	options_type *game_options;
	int res;

	// apply selecting BIOS
	game_options = GetGameOptions(game);
	options.bios = game_options->bios;

	audit_records = audit_images(game, AUDIT_VALIDATE_FAST, &audit);
	res = ProcessAuditResults(game, audit, audit_records);
	if (audit_records > 0)
		free(audit);

	SetRomAuditResults(game, res);
	return res;
}

// Verifies the Sample set while calling SetSampleAuditResults
int Mame32VerifySampleSet(int game)
{
	audit_record *audit;
	int audit_records;
	int res;

	audit_records = audit_samples(game, &audit);
	res = ProcessAuditResults(game, audit, audit_records);
	if (audit_records > 0)
		free(audit);

	SetSampleAuditResults(game, res);
	return res;
}

static DWORD WINAPI AuditThreadProc(LPVOID hDlg)
{
	char buffer[200];

	while (!bCancel)
	{
		if (!bPaused)
		{
			if (rom_index != -1)
			{
				sprintf(buffer, _UI("Checking Game %s - %s"),
					drivers[rom_index]->name, UseLangList() ? _LST(drivers[rom_index]->description) : drivers[rom_index]->description);
				SetWindowText(hDlg, _Unicode(buffer));
				ProcessNextRom();
			}
			else
			{
				if (sample_index != -1)
				{
					sprintf(buffer, _UI("Checking Game %s - %s"),
						drivers[sample_index]->name, UseLangList() ? _LST(drivers[sample_index]->description) : drivers[sample_index]->description);
					SetWindowText(hDlg, _Unicode(buffer));
					ProcessNextSample();
				}
				else
				{
					sprintf(buffer, "%s", _UI("File Audit"));
					SetWindowText(hDlg, _Unicode(buffer));
					EnableWindow(GetDlgItem(hDlg, IDPAUSE), FALSE);
					ExitThread(1);
				}
			}
		}
	}
	return 0;
}

static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread;
	static DWORD dwThreadID;
	DWORD dwExitCode;
	HWND hEdit;

	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, TRUE);

		hAudit = hDlg;
		//RS 20030613 Set Bkg of RichEdit Ctrl
		hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
		if (hEdit != NULL)
		{
			SendMessage( hEdit, EM_SETBKGNDCOLOR, FALSE, GetSysColor(COLOR_BTNFACE) );
			//RS Standard is MAX_UINT, which seems not to be enough, so we use a higher Number...
			SendMessage( hEdit, EM_EXLIMITTEXT, 0, MAX_TEXT );
		}
		SendDlgItemMessage(hDlg, IDC_ROMS_PROGRESS,    PBM_SETRANGE, 0, MAKELPARAM(0, GetNumGames()));
		SendDlgItemMessage(hDlg, IDC_SAMPLES_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, GetNumGames()));
		bPaused = FALSE;
		bCancel = FALSE;
		rom_index = 0;
		hThread = CreateThread(NULL, 0, AuditThreadProc, hDlg, 0, &dwThreadID);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
            bPaused = FALSE;
			if (hThread)
			{
				bCancel = TRUE;
				if (GetExitCodeThread(hThread, &dwExitCode) && dwExitCode == STILL_ACTIVE)
				{
					PostMessage(hDlg, WM_COMMAND, wParam, lParam);
					return 1;
				}
				CloseHandle(hThread);
			}
			EndDialog(hDlg,0);
			break;

		case IDPAUSE:
			if (bPaused)
			{
				SetDlgItemText(hAudit, IDPAUSE, _Unicode(_UI("&Pause")));
				bPaused = FALSE;
			}
			else
			{
				SetDlgItemText(hAudit, IDPAUSE, _Unicode(_UI("&Continue")));
				bPaused = TRUE;
			}
			break;
		}
		return 1;
	}
	return 0;
}

/* Callback for the Audit property sheet */
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, TRUE);

#ifdef TREE_SHEET
		if (GetShowTreeSheet())
		{
			extern void ModifyPropertySheetForTreeSheet(HWND);
			ModifyPropertySheetForTreeSheet(hDlg);
		}
#endif /* TREE_SHEET */

		FlushFileCaches();
		hAudit = hDlg;
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(rom_index));
		SetTimer(hDlg, 0, 1, NULL);
		return 1;

	case WM_TIMER:
		KillTimer(hDlg, 0);
		{
			int iStatus;
			LPCSTR lpStatus;

			iStatus = Mame32VerifyRomSet(rom_index);
			lpStatus = DriverUsesRoms(rom_index) ? StatusString(iStatus) : _UI("None required");
			SetWindowText(GetDlgItem(hDlg, IDC_PROP_ROMS), _Unicode(lpStatus));

			iStatus = Mame32VerifySampleSet(rom_index);
			lpStatus = DriverUsesSamples(rom_index) ? StatusString(iStatus) : _UI("None required");
			SetWindowText(GetDlgItem(hDlg, IDC_PROP_SAMPLES), _Unicode(lpStatus));
		}
		ShowWindow(hDlg, SW_SHOW);
		break;
	}
	return 0;
}

static void ProcessNextRom()
{
	int retval;
	char buffer[200];

	retval = Mame32VerifyRomSet(rom_index);
	switch (retval)
	{
	case BEST_AVAILABLE: /* correct, incorrect or separate count? */
	case CORRECT:
		roms_correct++;
		sprintf(buffer, "%i", roms_correct);
		SetDlgItemText(hAudit, IDC_ROMS_CORRECT, _Unicode(buffer));
		sprintf(buffer, "%i", roms_correct + roms_incorrect);
		SetDlgItemText(hAudit, IDC_ROMS_TOTAL, _Unicode(buffer));
		break;

	case NOTFOUND:
		break;

	case INCORRECT:
		roms_incorrect++;
		sprintf(buffer, "%i", roms_incorrect);
		SetDlgItemText(hAudit, IDC_ROMS_INCORRECT, _Unicode(buffer));
		sprintf(buffer, "%i", roms_correct + roms_incorrect);
		SetDlgItemText(hAudit, IDC_ROMS_TOTAL, _Unicode(buffer));
		break;
	}

	rom_index++;
	SendDlgItemMessage(hAudit, IDC_ROMS_PROGRESS, PBM_SETPOS, rom_index, 0);

	if (rom_index == GetNumGames())
	{
		sample_index = 0;
		rom_index = -1;
	}
}

static void ProcessNextSample()
{
	int  retval;
	char buffer[200];
	
	retval = Mame32VerifySampleSet(sample_index);
	
	switch (retval)
	{
	case CORRECT:
		if (DriverUsesSamples(sample_index))
		{
			samples_correct++;
			sprintf(buffer, "%i", samples_correct);
			SetDlgItemText(hAudit, IDC_SAMPLES_CORRECT, _Unicode(buffer));
			sprintf(buffer, "%i", samples_correct + samples_incorrect);
			SetDlgItemText(hAudit, IDC_SAMPLES_TOTAL, _Unicode(buffer));
			break;
		}

	case NOTFOUND:
		break;
			
	case INCORRECT:
		samples_incorrect++;
		sprintf(buffer, "%i", samples_incorrect);
		SetDlgItemText(hAudit, IDC_SAMPLES_INCORRECT, _Unicode(buffer));
		sprintf(buffer, "%i", samples_correct + samples_incorrect);
		SetDlgItemText(hAudit, IDC_SAMPLES_TOTAL, _Unicode(buffer));
		
		break;
	}

	sample_index++;
	SendDlgItemMessage(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETPOS, sample_index, 0);
	
	if (sample_index == GetNumGames())
	{
		DetailsPrintf(_UI("Audit complete.\n"));
		SetDlgItemText(hAudit, IDCANCEL, _Unicode(_UI("&Close")));
		sample_index = -1;
	}
}

static void CLIB_DECL DetailsPrintf(const char *fmt, ...)
{
	HWND	hEdit;
	va_list marker;
	char	buffer[2000];
	char * s;
	long l;

	//RS 20030613 Different Ids for Property Page and Dialog
	// so see which one's currently instantiated
	hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
	if (hEdit ==  NULL)
		hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS_PROP);
	
	if (hEdit == NULL)
	{
		dprintf("audit detailsprintf() can't find any audit control");
		return;
	}

	va_start(marker, fmt);
	
	vsprintf(buffer, _(fmt), marker);
	
	va_end(marker);

	s = ConvertToWindowsNewlines(buffer);

	l = Edit_GetTextLength(hEdit);
	Edit_SetSel(hEdit, l, l);
	SendMessageW( hEdit, EM_REPLACESEL, FALSE, (LPARAM)(void *)_Unicode(s));
}

static const char * StatusString(int iStatus)
{
	static const char *ptr;

	ptr = _UI("Unknown");

	switch (iStatus)
	{
	case CORRECT:
		ptr = _UI("Passed");
		break;
		
	case BEST_AVAILABLE:
		ptr = _UI("Best available");
		break;
		
	case NOTFOUND:
		ptr = _UI("Not found");
		break;
		
	case INCORRECT:
		ptr = _UI("Failed");
		break;
	}

	return ptr;
}