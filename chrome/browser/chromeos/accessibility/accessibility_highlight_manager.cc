// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "chrome/browser/chromeos/accessibility/accessibility_highlight_manager.h"
#include "chrome/browser/chromeos/ui/accessibility_focus_ring_controller.h"
#include "content/public/browser/focused_node_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "ui/aura/window_tree_host.h"
#include "ui/wm/core/cursor_manager.h"

namespace chromeos {

namespace {

const gfx::Rect& OffscreenRect() {
  CR_DEFINE_STATIC_LOCAL(const gfx::Rect, r, (INT_MIN, INT_MIN, 0, 0));
  return r;
}

const gfx::Point& OffscreenPoint() {
  CR_DEFINE_STATIC_LOCAL(const gfx::Point, p, (INT_MIN, INT_MIN));
  return p;
}

ui::InputMethod* GetInputMethod(aura::Window* root_window) {
  if (root_window->GetHost())
    return root_window->GetHost()->GetInputMethod();
  return nullptr;
}

}  // namespace

AccessibilityHighlightManager::AccessibilityHighlightManager() {
  ash::Shell* shell = ash::Shell::GetInstance();
  shell->AddPreTargetHandler(this);
  shell->cursor_manager()->AddObserver(this);
  registrar_.Add(this, content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE,
                 content::NotificationService::AllSources());
  aura::Window* root_window = ash::Shell::GetPrimaryRootWindow();
  ui::InputMethod* input_method = GetInputMethod(root_window);
  input_method->AddObserver(this);
  focus_rect_ = OffscreenRect();
  cursor_point_ = OffscreenPoint();
  caret_point_ = OffscreenPoint();
}

AccessibilityHighlightManager::~AccessibilityHighlightManager() {
  // No need to do anything during shutdown
  if (!ash::Shell::HasInstance())
    return;

  ash::Shell* shell = ash::Shell::GetInstance();
  if (shell) {
    shell->RemovePreTargetHandler(this);
    shell->cursor_manager()->RemoveObserver(this);

    AccessibilityFocusRingController::GetInstance()->SetFocusRing(
        std::vector<gfx::Rect>(),
        AccessibilityFocusRingController::FADE_OUT_FOCUS_RING);
    AccessibilityFocusRingController::GetInstance()->SetCaretRing(
        OffscreenPoint());
    AccessibilityFocusRingController::GetInstance()->SetCursorRing(
        OffscreenPoint());

    aura::Window* root_window = shell->GetPrimaryRootWindow();
    ui::InputMethod* input_method = GetInputMethod(root_window);
    input_method->RemoveObserver(this);
  }
}

void AccessibilityHighlightManager::HighlightFocus(bool focus) {
  focus_ = focus;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightManager::HighlightCursor(bool cursor) {
  cursor_ = cursor;
  UpdateCursorHighlight();
}

void AccessibilityHighlightManager::HighlightCaret(bool caret) {
  caret_ = caret;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightManager::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_MOVED) {
    cursor_point_ = event->root_location();
    UpdateCursorHighlight();
  }
}

void AccessibilityHighlightManager::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() == ui::ET_KEY_PRESSED)
    UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightManager::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE);
  content::FocusedNodeDetails* node_details =
      content::Details<content::FocusedNodeDetails>(details).ptr();
  focus_rect_ = node_details->node_bounds_in_screen;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightManager::OnTextInputStateChanged(
    const ui::TextInputClient* client) {
  if (!client || client->GetTextInputType() == ui::TEXT_INPUT_TYPE_NONE) {
    caret_visible_ = false;
    UpdateFocusAndCaretHighlights();
  }
}

void AccessibilityHighlightManager::OnCaretBoundsChanged(
    const ui::TextInputClient* client) {
  gfx::Rect caret_bounds = client->GetCaretBounds();
  caret_point_ = caret_bounds.CenterPoint();
  caret_visible_ = client->GetTextInputType() != ui::TEXT_INPUT_TYPE_NONE &&
                   (caret_bounds.width() || caret_bounds.height());
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightManager::OnCursorVisibilityChanged(bool is_visible) {
  UpdateCursorHighlight();
}

void AccessibilityHighlightManager::UpdateFocusAndCaretHighlights() {
  auto controller = AccessibilityFocusRingController::GetInstance();

  // The caret highlight takes precedence over the focus highlight if
  // both are visible.
  if (caret_ && caret_visible_) {
    controller->SetCaretRing(caret_point_);
    controller->SetFocusRing(
        std::vector<gfx::Rect>(),
        AccessibilityFocusRingController::FADE_OUT_FOCUS_RING);
  } else if (focus_) {
    controller->SetCaretRing(OffscreenPoint());
    std::vector<gfx::Rect> rects;
    if (!focus_rect_.IsEmpty())
      rects.push_back(focus_rect_);
    controller->SetFocusRing(
        rects, AccessibilityFocusRingController::FADE_OUT_FOCUS_RING);
  } else {
    controller->SetCaretRing(OffscreenPoint());
    controller->SetFocusRing(
        std::vector<gfx::Rect>(),
        AccessibilityFocusRingController::FADE_OUT_FOCUS_RING);
  }
}

void AccessibilityHighlightManager::UpdateCursorHighlight() {
  gfx::Point point = cursor_point_;

  if (!cursor_)
    point = OffscreenPoint();

  if (!ash::Shell::GetInstance()->cursor_manager()->IsCursorVisible())
    point = OffscreenPoint();

  AccessibilityFocusRingController::GetInstance()->SetCursorRing(point);
}

}  // namespace chromeos