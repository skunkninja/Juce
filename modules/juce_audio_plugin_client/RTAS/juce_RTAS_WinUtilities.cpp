/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "../../juce_core/system/juce_TargetPlatform.h"
#include "../utility/juce_CheckSettingMacros.h"

// (these functions are in their own file because of problems including windows.h
// at the same time as the Digi headers)

#if JucePlugin_Build_RTAS

#define _DO_NOT_DECLARE_INTERLOCKED_INTRINSICS_IN_MEMORY // (workaround for a VC build problem)

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#undef STRICT
#define STRICT
#include <intrin.h>
#include <windows.h>

#pragma pack (push, 8)
#include "../utility/juce_IncludeModuleHeaders.h"
#pragma pack (pop)

//==============================================================================
void JUCE_CALLTYPE attachSubWindow (void* hostWindow,
                                    int& titleW, int& titleH,
                                    Component* comp)
{
    RECT clientRect;
    GetClientRect ((HWND) hostWindow, &clientRect);

    titleW = clientRect.right - clientRect.left;
    titleH = jmax (0, (int) (clientRect.bottom - clientRect.top) - comp->getHeight());
    comp->setTopLeftPosition (0, titleH);

    comp->addToDesktop (0);

    HWND plugWnd = (HWND) comp->getWindowHandle();
    SetParent (plugWnd, (HWND) hostWindow);

    DWORD val = GetWindowLong (plugWnd, GWL_STYLE);
    val = (val & ~WS_POPUP) | WS_CHILD;
    SetWindowLong (plugWnd, GWL_STYLE, val);

    val = GetWindowLong ((HWND) hostWindow, GWL_STYLE);
    SetWindowLong ((HWND) hostWindow, GWL_STYLE, val | WS_CLIPCHILDREN);
}

void JUCE_CALLTYPE resizeHostWindow (void* hostWindow,
                                     int& titleW, int& titleH,
                                     Component* comp)
{
    RECT clientRect, windowRect;
    GetClientRect ((HWND) hostWindow, &clientRect);
    GetWindowRect ((HWND) hostWindow, &windowRect);
    const int borderW = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    const int borderH = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

    SetWindowPos ((HWND) hostWindow, 0, 0, 0,
                  borderW + jmax (titleW, comp->getWidth()),
                  borderH + comp->getHeight() + titleH,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

#if ! JucePlugin_EditorRequiresKeyboardFocus

namespace
{
    HWND findMDIParentOf (HWND w)
    {
        const int frameThickness = GetSystemMetrics (SM_CYFIXEDFRAME);

        while (w != 0)
        {
            HWND parent = GetParent (w);

            if (parent == 0)
                break;

            TCHAR windowType [32] = { 0 };
            GetClassName (parent, windowType, 31);

            if (String (windowType).equalsIgnoreCase ("MDIClient"))
            {
                w = parent;
                break;
            }

            RECT windowPos, parentPos;
            GetWindowRect (w, &windowPos);
            GetWindowRect (parent, &parentPos);

            int dw = (parentPos.right - parentPos.left) - (windowPos.right - windowPos.left);
            int dh = (parentPos.bottom - parentPos.top) - (windowPos.bottom - windowPos.top);

            if (dw > 100 || dh > 100)
                break;

            w = parent;

            if (dw == 2 * frameThickness)
                break;
        }

        return w;
    }
}

void JUCE_CALLTYPE passFocusToHostWindow (void* hostWindow)
{
    SetFocus (findMDIParentOf ((HWND) hostWindow));
}

#endif
#endif
