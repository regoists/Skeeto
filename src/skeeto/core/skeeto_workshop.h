#pragma once

extern const char kSkeetoWorkshopIdDefault[];

void SkeetoWorkshopUpdatePump();
void SkeetoWorkshopUpdateShutdown();
void SkeetoWorkshopTryOpenChangelog();
void SkeetoBootstrapMigrationNoticeOnce();
// A null tip is a background request: failure must not block on a dialog.
bool SkeetoOpenRootDoc(const char* utf8Leaf, const wchar_t* missingTip);
