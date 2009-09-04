<html>
<head>
<?cs include:"includes.cs" ?>
<link rel="stylesheet" type="text/css" href="templates/<?cs var:CGI.cur.device_style?>">
<script type="text/javascript" src="jslib/map_viewer.js"></script>
</head>

<body onload="ros_handleOnLoad('/ros')">
<?cs include:"header.cs" ?>
<h3>Navigation</h3>

<table align=right>
<td>
<div class=app_button objtype="LaunchButtonWidget2" taskid="navigation_application/navigation_application.app">
</td>
</table>

<table width=80% align=center>
<tr>
<td>
<div class=mapviewer objtype=MapViewer></div><br>
</td>
</table>

<div id=ErrorDiv></div>

</body>
</html>
