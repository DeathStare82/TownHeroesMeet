#define _CRT_SECURE_NO_WARNINGS

#include "homm3.h"
#include "era.h"
#include "patcher_x86.hpp"

using namespace Era;

Patcher* _P;
PatcherInstance* _PI;

bool inTownDlg;

// 새 버튼에 부여할 고유 ID (타운 다이얼로그 내부에서 중복되지 않는 ID)
#define BTN_HEROES_MEET_ID 8888

// 타운 메시지 프로시저 후킹
int __stdcall Y_DlgTown_Proc(HiHook* hook, _TownMgr_* tm, _EventMsg_* msg)
{
    int result = CALL_2(int, __thiscall, hook->GetDefaultFunc(), tm, msg);

    inTownDlg = false;

    if (result) 
    {
        // 1. 타운 화면 진입 및 갱신 시 버튼이 없으면 추가 생성
        if (tm && tm->dlg) 
        {
            _DlgItem_* existingBtn = tm->dlg->GetItem(BTN_HEROES_MEET_ID);
            
            // 두 영웅 존재 여부 확인
            int heroU_id = tm->town->up_hero_id;
            int heroD_id = tm->town->down_hero_id;
            bool bothHeroesExist = (heroU_id != -1 && heroD_id != -1);

            // 상단/하단 영웅이 모두 있을 때만 버튼 표시
            if (bothHeroesExist) 
            {
                if (!existingBtn) 
                {
                    // 두 영웅 초상화 사이 위치 (X: 302, Y: 295 - 타운 레이아웃 기준 좌표)
                    // 교류 아이콘 DEF 파일: "i_meet.def" 또는 "i_swap.def" (기본 게임 UI 리소스 사용)
                    _DlgButton_* meetBtn = _DlgButton_::Create(302, 295, 32, 32, BTN_HEROES_MEET_ID, "i_swap.def", 0, 1, 0, 28, 0);
                    if (meetBtn) 
                    {
                        tm->dlg->AddItem(meetBtn);
                        tm->dlg->Redraw();
                    }
                }
            } 
            else if (existingBtn) 
            {
                // 영웅이 한 명이라도 나가면 버튼 제거
                tm->dlg->RemoveItem(existingBtn);
                delete existingBtn;
                tm->dlg->Redraw();
            }
        }

        // 2. 마우스 클릭 이벤트 처리 (버튼 클릭 시)
        if (msg->type == MT_MOUSECLICK && msg->subtype == BTN_HEROES_MEET_ID) 
        {
            int heroU_id = tm->town->up_hero_id;
            int heroD_id = tm->town->down_hero_id;

            if (heroU_id != -1 && heroD_id != -1) 
            {
                _Hero_* heroU = o_GameMgr->GetHero(heroU_id);
                _Hero_* heroD = o_GameMgr->GetHero(heroD_id);

                inTownDlg = true;

                if (*(int*)((int)o_ExecMgr + 4) != (int)o_WndMgr) 
                {
                    *(int*)((int)o_TownMgr + 4) = (int)o_AdvMgr;
                    *(int*)((int)o_TownMgr + 8) = (int)o_WndMgr;

                    *(int*)((int)o_AdvMgr + 4) = NULL;
                    *(int*)((int)o_AdvMgr + 8) = (int)o_WndMgr;            

                    *(int*)((int)o_WndMgr + 4) = (int)o_TownMgr;
                    *(int*)((int)o_WndMgr + 8) = (int)o_MouseMgr;
                }

                heroU->TeachScholar(heroD);
                o_AdvMgr->SwapHeroes(heroU, heroD);

                o_TownMgr->UnHighlightArmy();        
                o_TownMgr->Redraw();    

                inTownDlg = false;
            }
        }
    }

    return result;
}

int __stdcall Y_Dlg_HeroesMeet(LoHook* h, HookContext* c)
{    
    if (inTownDlg) { 
        c->return_address = 0x4AAC2A;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

void Dlg_TownHeroesMeet(PatcherInstance* _PI)
{
    _PI->WriteHiHook(0x5D3640, SPLICE_, EXTENDED_, THISCALL_, Y_DlgTown_Proc);
    _PI->WriteLoHook(0x4AAC1B, Y_Dlg_HeroesMeet);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool plugin_On = false;

    if (ul_reason_for_call == DLL_PROCESS_ATTACH && !plugin_On)
    {
        plugin_On = true;

        _P = GetPatcher();
        _PI = _P->CreateInstance("TownHeroesMeetPlugin");

        ConnectEra();
        Dlg_TownHeroesMeet(_PI);
    }
    return TRUE;
}
