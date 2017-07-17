<html>
<head>
<link rel="shortcut icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/ietf-icon-blue3.png">
<link rel="apple-touch-icon" type="image/png" href="https://www.ietf.org/lib/dt/6.56.0/ietf/images/apple-touch-icon.png">
<title>Captive Portal for smart.mpvd.io</title>
</head>
<body>
<img src="ietflogo.gif"><br/>
<?php
require_once '../agent.php' ;
$questions = ['How many bits is there in an IPv6 address',
		'What is the protocol used in IPv6 to delegate a prefix',
		'What is the new RFC number for  draft-ietf-6man-rfc2460bis',
		'What is the link-local multicast for all nodes in IPv6'] ;
$answers = ['128',
	'DHCP-PD',
	'8200',
	'ff02::1'] ;
if (isset($_REQUEST['answer']) and isset($_REQUEST['id']) and is_numeric($_REQUEST['id'])) {
	$id = $_REQUEST['id'] ;
	if (strtolower($_REQUEST['answer']) == strtolower($answers[$id])) {
		if (agent_allow($_SERVER['REMOTE_ADDR']))
			print('<div style="color: blue;">As you are smart, you are now connected to the smart.pvd.io proviosionning domain.</div>') ;
		else
			print('<div style="color: red;">Good answer, but, cannot connect to our agent on your AP. Sorry.</div>') ;
	} else {
		print("Wrong answer, please try again") ;
	}
} 
$i = rand(0, sizeof($questions) - 1) ;
$question = $questions[$i] ;
?>
<h1>To connect to the smart PvD, answer this challenge</h1>
<form action=<?=$_SERVER['PHP_SELF']?>>
<input type="hidden" name="id" value="<?=$i?>">
<?=$question?>: <input type="text" length="4" name="answer">
<br>
<input type="submit" value="Submit">
</form>
</body>
</html>
