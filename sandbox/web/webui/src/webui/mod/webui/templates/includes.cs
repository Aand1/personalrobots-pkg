<link rel="stylesheet" type="text/css" href="<?cs var:CGI.ScriptName?>/webui/templates/<?cs var:CGI.cur.device_style?>">

<title>RosWeb: <?cs var:CGI.ServerName?></title>

<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/prototype.js"></script>
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/xss.js"></script>
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/ros.js"></script> 
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/ros_toolbar.js"></script> 
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/pr2_graph.js"></script> 
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/pr2_pb.js"></script> 
<script type="text/javascript" src="<?cs var:CGI.ScriptName?>/webui/jslib/pr2_widgets.js"></script> 

<?cs if:0 ?>
  <script type='text/javascript' src='http://www.google.com/jsapi'></script>
  <script type='text/javascript'>
    google.load("visualization", "1", {packages:["linechart"]});
    google.load("visualization", "1", {packages:["gauge"]});
  </script>
<?cs /if ?>
