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
--><?php
$ssh_host = 'lede.mpvd.io' ;
$ssh_port = 22 ;
$ssh_username = 'root' ;
$ssh_password = 'pvd' ;

function agent_allow($ipv6_address) {
	global $ssh_host, $ssh_port, $ssh_username, $ssh_password ;

	print("Configuring the hotspot based on your IPv6 address ($ipv6_address).<br/>Connecting to your captive portal controller... ") ;
	$ssh = ssh2_connect($ssh_host, $ssh_port) ;
	if ($ssh) {
		print("connected... ") ;
		if (ssh2_auth_password($ssh, $ssh_username, $ssh_password) === true)
			print("authenticated... ") ;
		else {
			print("<br/>Cannot authenticate...<br/>") ;
			return false ;
		}
		$stream = ssh2_exec($ssh, "/root/capport/agent.sh $ipv6_address") ;
		if ($stream === false) {
			print("<br/>Failed to execute a command.") ; 
			return false ;
		}
		$errorStream = ssh2_fetch_stream($stream, SSH2_STREAM_STDERR); 
		stream_set_blocking($errorStream, true);
		stream_set_blocking($stream, true);
		print ("<pre>" . stream_get_contents($stream) . '</pre><span style="color:red;"><pre>' . stream_get_contents($errorStream) . '</pre></span>');
		fclose($stream) ;
		return true ;
	} else {
		print("<br/>Failed to connect !<br/>") ; 
		return false ;
	}
} 
?>
