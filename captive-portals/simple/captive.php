<html>
<head>
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
<title>Captive Portal for simple.mpvd.io</title>
<link rel="shortcut icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/ietf-icon-blue3.png">
<link rel="apple-touch-icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/apple-touch-icon.png">
</head>
<body>
<img src="ietflogo.gif"><br/>
<?php
require_once '../agent.php' ;
if (agent_allow($_SERVER['REMOTE_ADDR']))
	print('<div style="color: blue;">You are now connected to the simple.pvd.io proviosioning domain.</div>') ;
else
	print('<div style="color: red;">Technical issue: cannot connect to our agent on your AP. Sorry.</div>') ;
?>
<h1>You are connected: simple.mpvd.io</h1>
All access from <?=$_SERVER['REMOTE_ADDR']?> is now allowed.
</body>
</html>
