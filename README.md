# pvd-captive-portal

DetectCaptivePortal and DetectCaptivePortal.js are supposed to be started
when the user logs in in a X11 session.

On Ubuntu/unity, just start the command (DetectCaptivePortal) in the
user's .xsessionrc file (create it if needed and give executable rights).

Once downloaded, type 'make' to generate the bind.so library that will
be preloaded (LD\_PRELOAD) by firefox.

firefox is started by DetectCaptivePortal.js and will be provided a local
URL (localhost:8080). This local page is served by a http server started
when the user logs in.

Both the http server (pvdHttpServer.js) and the detection module
(DetectCaptivePortal.js) are started by this DetectCaptivePortal script.
It also kills any present server and detection module, so this is basically
used in a single environment.

The CaptivePortal.html page is the one served by the local http server.
It shows the list of advertised PvDs and allows authenticate to captive
portals (if any) and select a given PvD.
