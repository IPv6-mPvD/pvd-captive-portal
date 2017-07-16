#!/usr/bin/env node

var exec = require("child_process").exec;
var pvdd = require("pvdd");

var pvddCnx = null;
var allPvd = {};
var alreadyStarted = false;

function handleConnection() {
	allPvd = {};
        pvddCnx.getList();
        pvddCnx.subscribeNotifications();
        pvddCnx.subscribeAttribute("*");
        console.log("Connection established with pvdd");
}

function handleError(err) {
        console.log("Connection lost with pvdd (" + err.message + ")");
	alreadyStarted = false;
}

function handlePvdList(pvdList) {
	pvdList.forEach(function(pvdname) {
		if (allPvd[pvdname] == null) {
			allPvd[pvdname] = { captivePortal : null };
			pvddCnx.getAttributes(pvdname);
		}
	});
}

function handleDelPvd(pvdname) {
	allPvd[pvdname] = null;
}

function handlePvdAttributes(pvdname, attrs) {
	console.log("Attributes for " + pvdname + " = " +
			JSON.stringify(attrs, null, "\t"));

	var captivePortal =
		attrs.captivePortalURL ||
		(attrs.extraInfo ? attrs.extraInfo.captivePortalURL : null);

	if (captivePortal != null &&
	    allPvd[pvdname] != null &&
	    allPvd[pvdname].captivePortal != captivePortal) {
		if (! alreadyStarted) {
			exec("firefox -new-tab " + "http://localhost:8080 &");
			console.log("Starting firefox on " + "http://localhost:8080");
			alreadyStarted = true;
		}
	}
	allPvd[pvdname].captivePortal = captivePortal;
}

function Monitor() {
	pvddCnx = pvdd.connect({ autoReconnect : true });
	pvddCnx.on("connect", handleConnection);
	pvddCnx.on("error", handleError);
	pvddCnx.on("pvdList", handlePvdList);
	pvddCnx.on("delPvd", handleDelPvd);
	pvddCnx.on("pvdAttributes", handlePvdAttributes);
}

var Help = false;
var Verbose = false;
var DelayExpected = false;
var Delay = null;

process.argv.forEach(function(arg) {
	if (arg == "-h" || arg == "--help") {
		Help = true;
	} else
	if (arg == "-v" || arg == "--verbose") {
		Verbose = true;
	} else
	if (arg == "-d" || arg == "--delay") {
		DelayExpected = true;
	} else
	if (DelayExpected) {
		DelayExpected = false;
		Delay = parseInt(arg);
	}
});


if (Delay != null) {
	setTimeout(Monitor, Delay * 1000);
}
else {
	Monitor();
}


/* ex: set ts=8 noexpandtab wrap: */
