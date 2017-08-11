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
 * This script starts a server that can be used to monitor pvds via a browser
 */
const Net = require('net');
const http = require('http');
const os = require("os");
const fs = require('fs');
const dns = require('dns');
const exec = require("child_process").exec;
const spawn = require("child_process").spawn;
const EventEmitter = require('events').EventEmitter;

var WebSocketServer = require('websocket').server;

var pvdd = require("pvdd");

var pvdEmitter = new EventEmitter();

var CreateRoutes = false;	/* reflects --create-routes */
var Verbose = false;

var allPvd = {};
var currentPvdList = [];

var Port = parseInt(process.env["PVDD_PORT"]) || 10101;
var OutAddrFile = process.env["BIND_ADDR_FILE"] || "/tmp/pvd-addr";
var TmpAddrFile = OutAddrFile + "." + process.pid;

function dlog(s) {
	if (Verbose) {
		console.log(s);
	}
}

function UnlinkFile(FileName) {
	try {
		fs.unlinkSync(FileName);
	}
	catch (e) {
	}
}

/*
 * SelectPvd : very specific for the captive portal feature (HACK inside).
 * We want to generate a file that can be read by a preloaded library
 * running (so to say) firefox.
 * Implicit PvDs have no extra information. We use a different src address
 * selection algorithm
 *
 * The file is made of line formatted as follows :
 * <dest-v6> <src-address-v6>
 * <*> <src-address-v6>
 * <*> <src-address-v4>
 *
 * The file is updated by the SelectPvd() function (not overwritten, simply
 * updated) and ResetPvdSelection()
 * If Dest == null, it will be replaced by '*'
 * If Dest is already present in the file, it will be replaced by the new
 * selection rule
 *
 * Dest = '*' => the user wants to force all destinations to use this PvD
 *
 * This script keeps in memory a copy of the file
 *
 * The src-address is retrieved from :
 * + the attributes addresses array associated to the pvd (the 1st address
 * will be used)
 * + the 1st address found (using the ip -6 addr command) matching one of
 * the PvD prefixes
 * 	+ the prefixes from the attributes
 * 	+ or the prefixes found in the extra information JSON object
 */
var SrcAddressRules = [];

function WriteSrcRules() {
	/*
	 * Create a string from the SrcAddressRules table
	 */
	s = "";
	Object.keys(SrcAddressRules).forEach(function(k) {
		s += k + " " + SrcAddressRules[k].address + "\n";
	});

	UnlinkFile(OutAddrFile);
	UnlinkFile(TmpAddrFile);
	try {
		fs.writeFileSync(
			TmpAddrFile,
			s,
			{mode : "0644", encoding : "utf8", flag : "w"});
	} catch(e) {
	}
	try {
		fs.renameSync(TmpAddrFile, OutAddrFile);
	} catch(e) {
	}
}

function SelectPvd(Pvd, Dest) {
	var prefixes = null;

	console.log("SelectPvd(", Pvd, ", ", Dest, ")");

	if (Dest == null) {
		Dest = "*";
	}

	if (Dest == '*') {
		SrcAddressRules = {};	// reset the routing table (FIXME : we
					// should also reset the kernel table)
	}

	if (allPvd[Pvd] == null || (p = allPvd[Pvd].attributes) == null) {
		return;
	}

	/*
	 * Fetch prefixes from the attributes or from the JSON extra info
	 */
	if ((prefixes = p.prefixes) == null) {
		if ((p_ = p.extraInfo) != null) {
		    prefixes = p_.prefixes;
		}
	}

	/*
	 * Define a closure to create kernel routes (only if we know the
	 * LLA of the router and the associated local interface)
	 */
	if (CreateRoutes && p.lla != null && p.dev != null) {
		var CreateRoute = function(Dest) {
			exec("ip -6 route add " + Dest + "/128 gw " + p.lla + " dev " + p.dev);
		};
	}
	else {
		var CreateRoute = null;
	}

	/*
	 * Function fetching a source address to use to reach a given destination
	 * It only updates the internal SrcAddressRules table
	 */
	function AddDestPvd(Pvd, Dest, ipv6) {
		console.log("AddDestPvd(", Pvd, ", ", Dest, ", ", ipv6, ")");

		if (ipv6 && p.addresses != null && p.addresses.length > 0) {
			SrcAddressRules[Dest] = {
				address : p.addresses[0],
				pvd : Pvd
			};
			console.log("Found addresses");
			return;
		}

		if (p.implicit && p.dev != "<no dev>") {
			exec(__dirname + "/get-local-addr.sh " + p.dev)
				.stdout.on("data", function(data) {
					console.log(data);
					SrcAddressRules[Dest] = {
						address : data,
						pvd : Pvd
					};
				});
			return;
		}

		if (prefixes == null) {
			return;
		}

		exec(__dirname + "/get-addr-multi-prefixes.sh " + p.dev + " " + prefixes.join(" "))
			.stdout.on("data", function(data) {
				console.log(data);
				SrcAddressRules[Dest] = {
					address : data,
					pvd : Pvd
				};
			});
	}

	/*
	 * Special case for '*' (we don't want to perform any DNS resolution)
	 */
	if (Dest == "*") {
		AddDestPvd(Pvd, "*", true);
	}
	else {
		/*
		 * Set the list of DNS servers to the ones attached to the PvD
		 * (if any) [for some reason, the dns NPM package we are using
		 * does not have any setServers() method)
		 */
		/*
		if (p.rdnss != null && p.rdnss.length != 0) {
			dns.setServers(p.rdnss);
		}
		else {
			dns.setServers([]);
		}
		*/

		dns.resolve6(Dest, function(err, addresses) {
			if (err == null) {
				addresses.forEach(function(address) {
					if (CreateRoute != null) {
						CreateRoute(address);
					}
					AddDestPvd(Pvd, address, true);
				});
			}
		});
		dns.resolve4(Dest, function(err, addresses) {
			if (err == null) {
				addresses.forEach(function(address) {
					AddDestPvd(Pvd, address, false);
				});
			}
		});
	}

	WriteSrcRules();
}

function ResetPvdSelection(Pvd) {
	for (var Dest in SrcAddressRules) {
		if ((p = SrcAddressRules[Dest]) != null && p.pvd == Pvd) {
			delete(SrcAddressRules[Dest]);
		}
	}
	WriteSrcRules();
}

/*
 * Regular connection related functions. The regular connection will be used to send
 * queries (PvD list and attributes) and to receive replies/notifications
 */
var pvddCnx = pvdd.connect({ autoReconnect : true, port : Port });

pvddCnx.on("connect", function() {
	pvddCnx.subscribeNotifications();
	pvddCnx.subscribeAttribute("*");
	pvddCnx.getList();
	console.log("Regular connection established with pvdd");
});

pvddCnx.on("error", function(err) {
	dlog("Can not connect to pvdd on port " +
		Port + " (" + err.message + ")");
	allPvd = {};
});

pvddCnx.on("pvdList", function(pvdList) {
	pvdList.forEach(function(pvdId) {
		if (allPvd[pvdId] == null) {
			/*
			 * New Pvd : create an entry
			 */
			allPvd[pvdId] = { pvd : pvdId, attributes : {} };
		}
		/*
		 * Always retrieve its attributes
		 */
		pvddCnx.getAttributes(pvdId);
	});
	/*
	 * Always notify the new pvd list, even if it has not changed
	 */
	currentPvdList = pvdList;
	pvdEmitter.emit("pvdList", currentPvdList);
	dlog("New pvd list : " + JSON.stringify(allPvd, null, 4));
});

pvddCnx.on("delPvd", function(pvdId) {
	allPvd[pvdId] = null;
});

pvddCnx.on("pvdAttributes", function(pvdId, attrs) {
	/*
	 * UpdateAttributes : update the internal attributes structure for a
	 * given pvdId and notifies all websocket connections. This function
	 * is called when the attributes for the PvD have been received
	 */
	dlog("Attributes for " + pvdId + " = " + JSON.stringify(attrs, null, 8));

	if (allPvd[pvdId] != null) {
		allPvd[pvdId].attributes = attrs;

		pvdEmitter.emit("pvdAttributes", {
			pvd : pvdId,
			pvdAttributes : attrs
		});
	}
});

/*
 * Options parsing
 */
var Help = false;
var PortNeeded = false;
var FileNeeded  = false;
var HttpPort = 8080;
var FileHtml = __dirname + "/pvdClient.html";

process.argv.forEach(function(arg) {
	if (arg == "-h" || arg == "--help") {
		Help = true;
	} else
	if (arg == "-v" || arg == "--verbose") {
		Verbose = true;
	} else
	if (arg == "-c" || arg == "--create-routes") {
		CreateRoutes = true;
	} else
	if (arg == "-p" || arg == "--port") {
		PortNeeded = true;
	} else
	if (arg == "-f" || arg == "--file") {
		FileNeeded = true;
	} else
	if (PortNeeded) {
		HttpPort = arg;
		PortNeeded = false;
	} else
	if (FileNeeded) {
		FileHtml = arg;
		FileNeeded = false;
	}
});

if (Help) {
	console.log("pvdHttpServer [-h|--help] <option>*");
	console.log("with option :");
	console.log("\t-v|--verbose : outputs extra logs during operation");
	console.log("\t-p|--port #  : http port to listen on (default 8080)");
	console.log("\t-f|--file <path.html> : static file to serve");
	process.exit(0);
}

console.log("Listening on http port " + HttpPort + ", pvdd port " + Port);
console.log("Hostname : " + os.hostname());
console.log("Serving static file : " + FileHtml);

/*
 * =================================================
 * Server part : it provides web clients a regular
 * http as well as a websocket connection to retrieve
 * a static page (via http) and live notifications
 * via the websocket (for PvD related informations)
 */
var server = http.createServer(function(req, res) {
	var page = fs.readFileSync(FileHtml);
	res.writeHead(200);
	res.end(page);
});

server.listen(HttpPort, "::");

var ws = new WebSocketServer({ httpServer : server, autoAcceptConnections: true });

function Send2Client(conn, o) {
	dlog("Send2Client : " + JSON.stringify(o));
	conn.sendUTF(JSON.stringify(o));
}

ws.on('connect', function(conn) {
	console.log("New websocket client");

	Send2Client(conn, {
		what : "hostname",
		payload : { hostname : os.hostname() }
	});

	function pvdList(ev) {
		Send2Client(conn, {
			what : "pvdList",
			payload : { pvdList : ev }
		});
	}

	function pvdAttributes(ev) {
		Send2Client(conn, {
			what : "pvdAttributes",
			payload : {
				pvd : ev.pvd,
				pvdAttributes : ev.pvdAttributes
			}
		});
	}

	pvdEmitter.on("pvdList", pvdList);
	pvdEmitter.on("pvdAttributes", pvdAttributes);

	conn.on("message", function(m) {
		if (m.type == "utf8") {
			HandleMessage(conn, m.utf8Data);
		}
	});
	conn.on("close", function() {
		pvdEmitter.removeListener("pvdList", pvdList);
		pvdEmitter.removeListener("pvdAttributes", pvdAttributes);
		console.log("Connection closed");
	});

});

function HandleMessage(conn, m) {
	console.log("Message received : " + m);
	if (m == "PVD_GET_LIST") {
		Send2Client(conn, {
			what : "pvdList",
			payload : {
				pvdList : currentPvdList
			}
		});
	} else
	if (m == "PVD_GET_ATTRIBUTES") {
		for (var key in allPvd) {
			if ((p = allPvd[key]) != null) {
				Send2Client(conn, {
					what : "pvdAttributes",
					payload : {
						pvd : p.pvd,
						pvdAttributes : p.attributes
					}
				});
			}
		};
	} else
	if ((r = m.match(/PVD_SELECT_PVD +([^ ]+) (.*)/i)) != null) {
		SelectPvd(r[1], r[2]);
	} else
	if ((r = m.match(/PVD_SELECT_PVD (.*)/i)) != null) {
		SelectPvd(r[1], "*");
	} else
	if ((r = m.match(/PVD_UNSELECT_PVD (.*)/i)) != null) {
		ResetPvdSelection(r[1]);
	}
}


/*
allPvd["pirl.cisco.com"] = {
	attributes : {
		"dev" : "ens0p3",
		"prefixes" : [ "ff00:0000:0000:0000::1", "ff00:0000:0000:0000::2" ]
	}
};

SelectPvd("pirl.cisco.com", "google.fr");
*/

/* ex: set ts=8 noexpandtab wrap: */
