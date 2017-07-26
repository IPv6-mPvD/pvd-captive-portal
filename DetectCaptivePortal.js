#!/usr/bin/env node

var exec = require("child_process").exec;
var pvdd = require("pvdd");

var pvddCnx = null;
var alreadyStarted = false;

function handleConnection() {
        pvddCnx.getList();
        console.log("Connection established with pvdd");
}

function handleError(err) {
        console.log("Connection lost with pvdd (" + err.message + ")");
	alreadyStarted = false;
}

/*
 * As soon as we receive a non empty list of PvD,
 * we fire firefox
 */
function handlePvdList(pvdList) {
	if (pvdList.length != 0) {
		console.log("PvD list : ", pvdList);
		if (! alreadyStarted) {
			exec("LD_PRELOAD=" + __dirname + "/bind.so " +
				"firefox -new-tab http://localhost:8080 >/tmp/firefox.log 2>&1 &");
			console.log("Starting firefox on http://localhost:8080");
			alreadyStarted = true;
		}
	}
}

pvddCnx = pvdd.connect({ autoReconnect : true });
pvddCnx.on("connect", handleConnection);
pvddCnx.on("error", handleError);
pvddCnx.on("pvdList", handlePvdList);

/* ex: set ts=8 noexpandtab wrap: */
