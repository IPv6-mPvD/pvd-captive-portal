#!/usr/bin/env node

/*
	Copyright 2017 Cisco

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
/*
 * This script starts a server that can be used in simulation mode
 * to debug the pvd.json retrieval and monitoring feature
 *
 * It goes along with the pvdid-monitor companion daemon
 */
const http = require('http');

var CaptiveUrl = {};

var JSONresponse = {
	"id" : 0,
	"metered" : false,
	"characteristics" : {
		"maxThroughput" : { "down" : 2000000 },
		"minLatency" : { "up" : 0.1 }
	},
	"expires" : "2017-04-25T16:46:00Z"
};

http.createServer(function(req, res) {
	var pvd = req.url.slice(1);

	if ((url = CaptiveUrl[pvd]) == null) {
		delete JSONresponse.captivePortal;
	}
	else {
		JSONresponse.captivePortal = url;
	}

	res.writeHead(200);
	JSONresponse.name = pvd;
	res.end(JSON.stringify(JSONresponse, null, 12) + "\n");
}).listen(8000);

/*
 * Update the expiration date and the id every 2 minutes
 * To avoid race conditions, we will update the expiration
 * date 5 seconds before its actual expiration date (so,
 * when clients will request the new JSON, its expires
 * date will have been updated 5 seconds before, which
 * should be long enough for the http server to have
 * updated it)
 */
const PERIOD = 120 * 1000;

function UpdateJson(id) {
	var now = new Date(Date.now() + PERIOD + 5);
	JSONresponse.expires = now.toISOString();
	JSONresponse.id = id;
	setTimeout(UpdateJson, PERIOD, id + 1);
}

UpdateJson(0);

/*
 * Options parsing
 */
var PvdExpected = false;
var Help = false;

process.argv.forEach(function(arg) {
	if (arg == "-h" || arg == "--help") {
		Help = true;
	} else
	if (arg == "-v" || arg == "--verbose") {
		Verbose = true;
	} else
	if (arg == "-cp" || arg == "--captive-portal") {
		PvdExpected = true;
		
	} else
	if (PvdExpected) {
		PvdExpected = false;
		if ((r = arg.match(/([^:]+)@+(.+)/i)) != null) {
			CaptiveUrl[r[1]] = r[2];
		}
	}
});

if (Help) {
	console.log("httpServerCaptivePortal.js [-h|--help] <option>*");
	console.log("with option :");
	console.log("\t-v|--verbose : outputs extra logs during operation");
	console.log("\t-cp|--captive-portal <pvd>:<url> : pvd name and its captive portal url");
	console.log("\nThe captive portal option can be repeated");
	console.log("Its intent is to add a captivePortal field in the returned JSON structure for");
	console.log("the given pvd");

	process.exit(0);
}

if (Verbose) {
	console.log("Captive portals list : ");
	console.log(CaptiveUrl);
}

/* ex: set ts=8 noexpandtab wrap: */
