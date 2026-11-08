#pragma once
#include <atomic>

// Kill counters + persist flags. Paint/Event increment counters; this TU owns the file.
extern std::atomic<int> g_elimSi;
extern std::atomic<int> g_elimCi;
extern std::atomic<int> g_elimSiMelee;
extern std::atomic<int> g_elimSkeet;
extern std::atomic<int> g_elimMeleeSkeet;
extern std::atomic<int> g_elimHs;
extern std::atomic<int> g_elimHsPool;
extern std::atomic<int> g_elimTank;
extern std::atomic<int> g_elimWitch;
extern std::atomic<bool> g_elimPersistActive;
extern std::atomic<bool> g_elimPersistDirty;
extern std::atomic<bool> g_elimPersistLoadPending;
extern std::atomic<bool> g_elimPersistSavePending;
extern std::atomic<bool> g_elimPersistLoaded;

void ElimPersistLoad();
void ElimPersistSaveWorker();
void ElimPersistMarkChanged();
void ElimPersistDeactivate();
