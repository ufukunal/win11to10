#include "ScheduledStartup.h"

#include <windows.h>
#include <taskschd.h>
#include <oleauto.h>

#include <algorithm>
#include <string>
#include <utility>

namespace noya {
namespace {
template<class T> struct ComPtr {
  T* value{};
  ~ComPtr() { if (value) value->Release(); }
  T** put() { return &value; }
  T* operator->() const { return value; }
  ComPtr(const ComPtr&) = delete;
  ComPtr& operator=(const ComPtr&) = delete;
  ComPtr() = default;
};
VARIANT Index(LONG i) {
  VARIANT index{};
  VariantInit(&index);
  index.vt = VT_I4;
  index.lVal = i;
  return index;
}
struct Bstr {
  BSTR value{};
  explicit Bstr(const wchar_t* text) : value(SysAllocString(text)) {}
  ~Bstr() { SysFreeString(value); }
};
bool StartupTrigger(TASK_TRIGGER_TYPE2 type) {
  return type == TASK_TRIGGER_BOOT || type == TASK_TRIGGER_LOGON;
}
std::wstring ReadTaskName(IRegisteredTask* task) {
  BSTR raw{};
  if (FAILED(task->get_Path(&raw)) || !raw) return L"";
  std::wstring name(raw, SysStringLen(raw));
  SysFreeString(raw);
  return name;
}
bool HasStartupTrigger(IRegisteredTask* task) {
  ComPtr<ITaskDefinition> definition;
  if (FAILED(task->get_Definition(definition.put()))) return false;
  ComPtr<ITriggerCollection> triggers;
  if (FAILED(definition->get_Triggers(triggers.put()))) return false;
  LONG count{};
  if (FAILED(triggers->get_Count(&count))) return false;
  for (LONG i = 1; i <= count; ++i) {
    ComPtr<ITrigger> trigger;
    if (FAILED(triggers->get_Item(i, trigger.put()))) continue;
    TASK_TRIGGER_TYPE2 type{};
    if (SUCCEEDED(trigger->get_Type(&type)) && StartupTrigger(type))
      return true;
  }
  return false;
}
void ScanFolder(ITaskFolder* folder, ScheduledStartupInventory& output,
                unsigned& visitedFolders, unsigned& visitedTasks,
                unsigned depth) {
  // Bounded enumeration avoids expensive traversal on abnormal task trees.
  if (depth > 12 || ++visitedFolders > 256) {
    output.diagnostics += L"Görev klasörü tarama sınırına ulaşıldı.\r\n";
    return;
  }
  ComPtr<IRegisteredTaskCollection> tasks;
  if (SUCCEEDED(folder->GetTasks(TASK_ENUM_HIDDEN, tasks.put()))) {
    LONG count{};
    if (SUCCEEDED(tasks->get_Count(&count))) {
      for (LONG i = 1; i <= count && visitedTasks < 4096; ++i) {
        ++visitedTasks;
        ComPtr<IRegisteredTask> task;
        if (FAILED(tasks->get_Item(Index(i), task.put()))) continue;
        if (!HasStartupTrigger(task.value)) continue;
        const auto name = ReadTaskName(task.value);
        if (name.empty()) continue;
        VARIANT_BOOL enabled = VARIANT_FALSE;
        const bool hasEnabled = SUCCEEDED(task->get_Enabled(&enabled));
        const wchar_t* state = !hasEnabled ? L"durum bilinmiyor" :
            enabled == VARIANT_TRUE ? L"etkin" : L"devre dışı";
        output.entries.push_back(name + L" — " + state);
      }
      if (visitedTasks >= 4096)
        output.diagnostics += L"Görev tarama sınırına ulaşıldı.\r\n";
    }
  }
  ComPtr<ITaskFolderCollection> folders;
  if (FAILED(folder->GetFolders(0, folders.put()))) return;
  LONG count{};
  if (FAILED(folders->get_Count(&count))) return;
  for (LONG i = 1; i <= count && visitedFolders < 256; ++i) {
    ComPtr<ITaskFolder> child;
    if (SUCCEEDED(folders->get_Item(Index(i), child.put())))
      ScanFolder(child.value, output, visitedFolders, visitedTasks, depth + 1);
  }
}
}  // namespace

ScheduledStartupInventory ScanScheduledStartup() {
  ScheduledStartupInventory result;
  ComPtr<ITaskService> service;
  HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITaskService,
                                reinterpret_cast<void**>(service.put()));
  if (FAILED(hr)) {
    result.diagnostics = L"Görev Zamanlayıcı okunamadı (COM: " +
                         std::to_wstring(static_cast<unsigned long>(hr)) + L").";
    return result;
  }
  VARIANT empty{};
  VariantInit(&empty);
  hr = service->Connect(empty, empty, empty, empty);
  if (FAILED(hr)) {
    result.diagnostics = L"Görev Zamanlayıcı bağlantısı kurulamadı (COM: " +
                         std::to_wstring(static_cast<unsigned long>(hr)) + L").";
    return result;
  }
  ComPtr<ITaskFolder> root;
  Bstr rootPath(L"\\");
  hr = service->GetFolder(rootPath.value, root.put());
  if (FAILED(hr)) {
    result.diagnostics = L"Görev Zamanlayıcı kök klasörü okunamadı.";
    return result;
  }
  unsigned visitedFolders = 0, visitedTasks = 0;
  ScanFolder(root.value, result, visitedFolders, visitedTasks, 0);
  std::sort(result.entries.begin(), result.entries.end());
  return result;
}

bool ScheduledStartupModelSelfTest() {
  return StartupTrigger(TASK_TRIGGER_BOOT) &&
         StartupTrigger(TASK_TRIGGER_LOGON) &&
         !StartupTrigger(TASK_TRIGGER_DAILY) &&
         !StartupTrigger(TASK_TRIGGER_TIME);
}
}  // namespace noya
