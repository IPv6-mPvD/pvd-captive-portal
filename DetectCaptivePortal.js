#!/usr/bin/env node

var exec = require("child_process").exec;
var pvddCnx = require("pvdd").connect({ autoReconnect : true });

var allPvd = {};

pvddCnx.on("connect", function() {
	allPvd = {};
        pvddCnx.getList();
        pvddCnx.subscribeNotifications();
        pvddCnx.subscribeAttribute("*");
        console.log("Connection established with pvdd");
});
pvddCnx.on("error", function(err) {
        console.log("Connection lost with pvdd (" + err.message + ")");
});

pvddCnx.on("pvdList", function(pvdList) {
	pvdList.forEach(function(pvdname) {
		if (allPvd[pvdname] == null) {
			allPvd[pvdname] = { captivePortal : null };
			pvddCnx.getAttributes(pvdname);
		}
	});
});

pvddCnx.on("delPvd", function(pvdname) { allPvd[pvdname] = null; });

pvddCnx.on("pvdAttributes", function(pvdname, attrs) {
	console.log("Attributes for " + pvdname + " = " +
			JSON.stringify(attrs, null, "\t"));

	var captivePortal =
		attrs.captivePortalURL ||
		(attrs.extraInfo ? attrs.extraInfo.captivePortalURL : null);

	if (captivePortal != null &&
	    allPvd[pvdname] != null &&
	    allPvd[pvdname].captivePortal != captivePortal) {
		exec("firefox -new-tab " + captivePortal);
		console.log("Starting firefox on " + captivePortal);
	}
	allPvd[pvdname].captivePortal = captivePortal;
});
