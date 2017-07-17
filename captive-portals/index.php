<html>
<!--
/*
	Copyright 2017 Cisco
	Author: Eric Vyncke, 2017

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
-->
<head>
<title>Hackathon IETF-99 mPvD & Capport</title>
<link rel="shortcut icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/ietf-icon-blue3.png">
<link rel="apple-touch-icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/apple-touch-icon.png">
</head>
<body>
<h1>Hackathon IETF-99 mPvD & Capport</h1>
<?php
$addr = $_SERVER['REMOTE_ADDR'] ;
print("<p>Your source address is <b>$addr</b>.</p>") ;
if (strpos($addr, '2001:67c:1230:babe:') === 0) print("<p>You selected the SIMPLE PvD.</p>") ;
if (strpos($addr, '2001:67c:1230:abba:') === 0) print("<p>You selected the SMART PvD.</p>") ;
if (strpos($addr, '2001:67c:1230:bade:') === 0) print("<p>You selected the SMART PvD.</p>") ;
if (strpos($addr, '2001:67c:1230:baab:') === 0) print("<p>You selected the SIMPLE PvD.</p>") ;
if (strpos($addr, '2001:67c:1230:ebab:') === 0) print("<p>You selected the JAIL PvD.</p>") ;
if (strpos($addr, '2001:67c:1230:b0b0:') === 0) print("<p>You selected the JAIL PvD.</p>") ;
?>
<img src="smart/mpvd.png">
</body>
</html>
