// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/common/session/session_state_delegate.h"
#include "ash/shell.h"
#include "base/command_line.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/common/chrome_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace chromeos {

namespace {

const char kTestUser[] = "test-user@gmail.com";

}  // namespace

class BrowserLoginTest : public chromeos::LoginManagerTest {
 public:
  BrowserLoginTest() : LoginManagerTest(true) {}
  ~BrowserLoginTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    LoginManagerTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(::switches::kCreateBrowserOnStartupForTests);
  }
};

IN_PROC_BROWSER_TEST_F(BrowserLoginTest, PRE_BrowserActive) {
  RegisterUser(kTestUser);
  EXPECT_EQ(ash::SessionStateDelegate::SESSION_STATE_LOGIN_PRIMARY,
            ash::Shell::GetInstance()->session_state_delegate()->
                GetSessionState());
  chromeos::StartupUtils::MarkOobeCompleted();
}

IN_PROC_BROWSER_TEST_F(BrowserLoginTest, BrowserActive) {
  EXPECT_EQ(ash::SessionStateDelegate::SESSION_STATE_LOGIN_PRIMARY,
            ash::Shell::GetInstance()->session_state_delegate()->
                GetSessionState());
  LoginUser(kTestUser);
  EXPECT_EQ(ash::SessionStateDelegate::SESSION_STATE_ACTIVE,
            ash::Shell::GetInstance()->session_state_delegate()->
                GetSessionState());

  Browser* browser =
      chrome::FindAnyBrowser(ProfileManager::GetActiveUserProfile(), false);
  EXPECT_TRUE(browser != NULL);
  EXPECT_TRUE(browser->window()->IsActive());

  gfx::NativeWindow window = browser->window()->GetNativeWindow();
  views::Widget* widget = views::Widget::GetWidgetForNativeWindow(window);
  views::FocusManager* focus_manager = widget->GetFocusManager();
  EXPECT_TRUE(focus_manager != NULL);

  const views::View* focused_view = focus_manager->GetFocusedView();
  EXPECT_TRUE(focused_view != NULL);
  EXPECT_EQ(VIEW_ID_OMNIBOX, focused_view->id());
}

}  // namespace chromeos
