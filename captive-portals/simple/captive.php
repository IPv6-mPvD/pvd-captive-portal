<html>
<head>
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
