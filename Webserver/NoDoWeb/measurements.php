<?php 
require_once('connections/tc.php'); 
require_once('include/auth.php'); 
require_once('include/settings.php'); 

$page_title = "Measurements";

//lees sensoren uit
mysql_select_db($database_tc, $tc);
$query_RSsensor = "SELECT * FROM nodo_tbl_sensor WHERE user_id='$userId'";
$RSsensor = mysql_query($query_RSsensor, $tc) or die(mysql_error());


?>



<!DOCTYPE html> 
<html> 

<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1"> 
	<title><?php echo $title ?></title> 
	<?php require_once('include/jquery_mobile.php'); ?>
	
</head> 

<body> 

<div data-role="page" data-theme="<?php echo $theme?>">

<?php require_once('include/header.php'); ?>

	<div data-role="content">	

<h4>	
	<?php	while($row_RSsensor = mysql_fetch_array($RSsensor))
	
	

	{
		
		echo $row_RSsensor['sensor_name'];
		echo " ";
		
		if ($row_RSsensor['display'] == 1){
			echo $row_RSsensor['data'];
			echo " ";
			echo $row_RSsensor['sensor_suffix'];
		}
		
		if ($row_RSsensor['display'] == 2){
			if ($row_RSsensor['data'] <=0){	echo $row_RSsensor['sensor_suffix_false'];}
			if ($row_RSsensor['data'] > 0){	echo $row_RSsensor['sensor_suffix_true'];}
		}
		
		echo "<br>";
		
	}
	
	?>
</h4>
	
		

</div><!-- /content -->
<?php require_once('include/footer.php'); ?>
	
</div><!-- /page -->

</body>
</html>