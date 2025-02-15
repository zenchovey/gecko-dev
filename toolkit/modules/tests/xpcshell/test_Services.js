/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

/**
 * This file tests the Services.jsm module.
 */

// //////////////////////////////////////////////////////////////////////////////
// / Globals

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function checkService(service, interface) {
  do_print("Checking that Services." + service + " is an " + interface);
  do_check_true(service in Services);
  do_check_true(Services[service] instanceof interface);
}

// //////////////////////////////////////////////////////////////////////////////
// / Tests

function run_test()
{
  do_get_profile();

  checkService("appShell", Ci.nsIAppShellService);
  checkService("appinfo", Ci.nsIXULRuntime);
  checkService("blocklist", Ci.nsIBlocklistService);
  checkService("cache", Ci.nsICacheService);
  checkService("cache2", Ci.nsICacheStorageService);
  checkService("clipboard", Ci.nsIClipboard);
  checkService("console", Ci.nsIConsoleService);
  checkService("contentPrefs", Ci.nsIContentPrefService);
  checkService("cookies", Ci.nsICookieManager2);
  checkService("dirsvc", Ci.nsIDirectoryService);
  checkService("dirsvc", Ci.nsIProperties);
  checkService("DOMRequest", Ci.nsIDOMRequestService);
  checkService("domStorageManager", Ci.nsIDOMStorageManager);
  checkService("downloads", Ci.nsIDownloadManager);
  checkService("droppedLinkHandler", Ci.nsIDroppedLinkHandler);
  checkService("eTLD", Ci.nsIEffectiveTLDService);
  checkService("focus", Ci.nsIFocusManager);
  checkService("io", Ci.nsIIOService);
  checkService("io", Ci.nsIIOService2);
  checkService("locale", Ci.nsILocaleService);
  checkService("logins", Ci.nsILoginManager);
  checkService("obs", Ci.nsIObserverService);
  checkService("perms", Ci.nsIPermissionManager);
  checkService("prefs", Ci.nsIPrefBranch);
  checkService("prefs", Ci.nsIPrefService);
  checkService("prompt", Ci.nsIPromptService);
  checkService("scriptSecurityManager", Ci.nsIScriptSecurityManager);
  checkService("scriptloader", Ci.mozIJSSubScriptLoader);
  checkService("startup", Ci.nsIAppStartup);
  checkService("storage", Ci.mozIStorageService);
  checkService("strings", Ci.nsIStringBundleService);
  checkService("sysinfo", Ci.nsIPropertyBag2);
  checkService("telemetry", Ci.nsITelemetry);
  checkService("tm", Ci.nsIThreadManager);
  checkService("uriFixup", Ci.nsIURIFixup);
  checkService("urlFormatter", Ci.nsIURLFormatter);
  checkService("vc", Ci.nsIVersionComparator);
  checkService("wm", Ci.nsIWindowMediator);
  checkService("ww", Ci.nsIWindowWatcher);
  if ("nsIBrowserSearchService" in Ci) {
    checkService("search", Ci.nsIBrowserSearchService);
  }
  if ("nsIAndroidBridge" in Ci) {
    checkService("androidBridge", Ci.nsIAndroidBridge);
  }

  // In xpcshell tests, the "@mozilla.org/xre/app-info;1" component implements
  // only the nsIXULRuntime interface, but not nsIXULAppInfo.  To test the
  // service getter for the latter interface, load mock app-info.
  let tmp = {};
  Cu.import("resource://testing-common/AppInfo.jsm", tmp);
  tmp.updateAppInfo();

  // We need to reload the module to update the lazy getter.
  Cu.unload("resource://gre/modules/Services.jsm");
  Cu.import("resource://gre/modules/Services.jsm");

  checkService("appinfo", Ci.nsIXULAppInfo);

  Cu.unload("resource://gre/modules/Services.jsm");
}
