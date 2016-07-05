// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_WS_USER_ID_TRACKER_OBSERVER_H_
#define COMPONENTS_MUS_WS_USER_ID_TRACKER_OBSERVER_H_

#include <stdint.h>

#include "components/mus/ws/user_id.h"

namespace mus {
namespace ws {

class UserIdTrackerObserver {
 public:
  virtual void OnActiveUserIdChanged(const UserId& previously_active_id,
                                     const UserId& active_id) = 0;
  virtual void OnUserIdAdded(const UserId& id) = 0;
  virtual void OnUserIdRemoved(const UserId& id) = 0;

 protected:
  virtual ~UserIdTrackerObserver() {}
};

}  // namespace ws
}  // namespace mus

#endif  // COMPONENTS_MUS_WS_USER_ID_TRACKER_OBSERVER_H_