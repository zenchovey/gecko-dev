/* -*- Mode: indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set sts=2 sw=2 et tw=80: */
"use strict";

add_task(function* setup() {
  // make sure userContext is enabled.
  return SpecialPowers.pushPrefEnv({"set": [
    ["privacy.userContext.enabled", true],
  ]});
});

add_task(function* () {
  info("Start testing tabs.create with cookieStoreId");

  let testCases = [
    // No private window
    {privateTab: false, cookieStoreId: null, success: true, expectedCookieStoreId: "firefox-default"},
    {privateTab: false, cookieStoreId: "firefox-default", success: true, expectedCookieStoreId: "firefox-default"},
    {privateTab: false, cookieStoreId: "firefox-container-1", success: true, expectedCookieStoreId: "firefox-container-1"},
    {privateTab: false, cookieStoreId: "firefox-container-2", success: true, expectedCookieStoreId: "firefox-container-2"},
    {privateTab: false, cookieStoreId: "firefox-container-42", failure: "exist"},
    {privateTab: false, cookieStoreId: "firefox-private", failure: "defaultToPrivate"},
    {privateTab: false, cookieStoreId: "wow", failure: "illegal"},

    // Private window
    {privateTab: true, cookieStoreId: null, success: true, expectedCookieStoreId: "firefox-private"},
    {privateTab: true, cookieStoreId: "firefox-private", success: true, expectedCookieStoreId: "firefox-private"},
    {privateTab: true, cookieStoreId: "firefox-default", failure: "privateToDefault"},
    {privateTab: true, cookieStoreId: "firefox-container-1", failure: "privateToDefault"},
    {privateTab: true, cookieStoreId: "wow", failure: "illegal"},
  ];

  let extension = ExtensionTestUtils.loadExtension({
    manifest: {
      "permissions": ["tabs", "cookies"],
    },

    background: function() {
      function testTab(data, tab) {
        browser.test.assertTrue(data.success, "we want a success");
        browser.test.assertTrue(!!tab, "we have a tab");
        browser.test.assertEq(data.expectedCookieStoreId, tab.cookieStoreId, "tab should have the correct cookieStoreId");
      }

      function runTest(data) {
        // Tab Creation
        browser.tabs.create({windowId: data.privateTab ? this.privateWindowId : this.defaultWindowId,
                             cookieStoreId: data.cookieStoreId})

        // Tests for tab creation
        .then((tab) => {
          testTab(data, tab);
          return tab;
        }, (error) => {
          browser.test.assertTrue(!!data.failure, "we want a failure");
          if (data.failure == "illegal") {
            browser.test.assertTrue(/Illegal cookieStoreId/.test(error.message),
                                    "runtime.lastError should report the expected error message");
          } else if (data.failure == "defaultToPrivate") {
            browser.test.assertTrue("Illegal to set private cookieStorageId in a non private window",
                                    error.message,
                                    "runtime.lastError should report the expected error message");
          } else if (data.failure == "privateToDefault") {
            browser.test.assertTrue("Illegal to set non private cookieStorageId in a private window",
                                    error.message,
                                    "runtime.lastError should report the expected error message");
          } else if (data.failure == "exist") {
            browser.test.assertTrue(/No cookie store exists/.test(error.message),
                                    "runtime.lastError should report the expected error message");
          } else {
            browser.test.fail("The test is broken");
          }

          return null;
        })

        // Tests for tab querying
        .then((tab) => {
          if (tab) {
            return browser.tabs.query({windowId: data.privateTab ? this.privateWindowId : this.defaultWindowId,
                                       cookieStoreId: data.cookieStoreId})
                   .then((tabs) => {
                     browser.test.assertTrue(tabs.length >= 1, "Tab found!");
                     testTab(data, tabs[0]);
                     return tab;
                   });
          }
        })

        .then((tab) => {
          if (tab) {
            return browser.cookies.getAllCookieStores()
                   .then(stores => {
                     let store = stores.find(store => store.id === tab.cookieStoreId);
                     browser.test.assertTrue(!!store, "We have a store for this tab.");
                     return tab;
                   });
          }
        })

        .then((tab) => {
          if (tab) {
            return browser.tabs.remove(tab.id);
          }
        })

        .then(() => {
          browser.test.sendMessage("test-done");
        }, () => {
          browser.test.fail("An exception has ben thrown");
        });
      }

      function initialize() {
        browser.windows.create({incognito: true})
        .then((win) => {
          this.privateWindowId = win.id;
          return browser.windows.create({incognito: false});
        })
        .then((win) => {
          this.defaultWindowId = win.id;
        })
        .then(() => {
          browser.test.sendMessage("ready");
        });
      }

      function shutdown() {
        browser.windows.remove(this.privateWindowId)
        .then(() => {
          browser.windows.remove(this.defaultWindowId);
        })
        .then(() => {
          browser.test.sendMessage("gone");
        });
      }

      // Waiting for messages
      browser.test.onMessage.addListener((msg, data) => {
        if (msg == "be-ready") {
          initialize();
        } else if (msg == "test") {
          runTest(data);
        } else {
          browser.test.assertTrue("finish", msg, "Shutting down");
          shutdown();
        }
      });
    },
  });

  yield extension.startup();

  info("Tests must be ready...");
  extension.sendMessage("be-ready");
  yield extension.awaitMessage("ready");
  info("Tests are ready to run!");

  for (let test of testCases) {
    info(`test tab.create with cookieStoreId: "${test.cookieStoreId}"`);
    extension.sendMessage("test", test);
    yield extension.awaitMessage("test-done");
  }

  info("Waiting for shutting down...");
  extension.sendMessage("finish");
  yield extension.awaitMessage("gone");

  yield extension.unload();
});
