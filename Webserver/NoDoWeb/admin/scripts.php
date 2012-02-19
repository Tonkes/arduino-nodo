<?php 

require_once('../connections/tc.php'); 
require_once('../include/auth.php');
require_once('../include/settings.php');

$page_title = "Setup: Scripts";	


/************************************************************************************************
HTTPRequest function														
*************************************************************************************************/
function HTTPRequest($Url){


    
    if (!function_exists('curl_init')){
        die('Sorry cURL is not installed!');
    }
 
    global $nodo_port;
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $Url);
    curl_setopt($ch, CURLOPT_USERAGENT, "NoDoWeb");
    curl_setopt($ch, CURLOPT_HEADER, 0);
	curl_setopt($ch, CURLOPT_PORT, $nodo_port);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_TIMEOUT, 5);
 
    $output = curl_exec($ch);
    curl_close($ch);
	
	return $output;
	
	}
/************************************************************************************************
END HTTPRequest function														
*************************************************************************************************/

/************************************************************************************************
Eventlist read own files														
*************************************************************************************************/

//HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=FileLog%20Files");
//HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=FileList");
//HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=FileLog");
/************************************************************************************************
END Eventlist read own files														
*************************************************************************************************/



/************************************************************************************************
Eventlist script read															
*************************************************************************************************/
if (isset($_POST['Read'])) 
{  


	if ($_POST['select-script-1'] == "EVENTLST") {
	
		$file = "EVENTLST"; 

		//Write eventlist on nodo
			
		HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=EventListfile%20$file");
		
	}
	else {
	
		
		$file = $_POST['select-script-1'];

	}

	//Read file from Nodo to array
	$scriptraw = explode("\n", HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&file=$file"));
	
		
	//count total lines in $scriptraw
	$total_script_lines_raw = count($scriptraw);
	
	$x=0;
	
	for($i=0;$i<$total_script_lines_raw;$i++){
		
		//Remove !********************************** start en stop lines 
		$pos = strpos($scriptraw[$i],"!*****");
        
		if($pos === false || $pos > 0) {
			$script[$x] = $scriptraw[$i];
			$x++;
		}
	}
	
	$total_script_lines = count($script);
	
    
 
 
}
/************************************************************************************************
END script Read															
*************************************************************************************************/


/************************************************************************************************
Script write or execute														
*************************************************************************************************/
if (isset($_POST['Write'])) 
{  

 
 // get form data, making sure it is valid 
 $file = mysql_real_escape_string(htmlspecialchars($_POST['select-script-1'])); 
 $scriptpost = mysql_real_escape_string(htmlspecialchars($_POST['script']));
 
 
 
 mysql_select_db($database_tc, $tc);
 $result = mysql_query("SELECT * FROM nodo_tbl_scripts WHERE user_id='$userId' AND file = '$file'") or die(mysql_error());  
 
	 if (mysql_affected_rows()==0) {
		// update the data to the database 
		 mysql_query("INSERT INTO nodo_tbl_scripts (script, file, user_id) 
		 VALUES 
		 ('$scriptpost','$file','$userId')");
		 
		     
	 
	 }
 
 
	else {

		// save the data to the database 
		 mysql_select_db($database_tc, $tc);
		 mysql_query("UPDATE NODO_tbl_scripts SET script='$scriptpost' WHERE user_id='$userId' AND file='$file'") or die(mysql_error());   
		  
		 
		
	}
			
		
	//Save script on Nodo 
	HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=FileGetHTTP%20$file");
	
	
	if (ISSET($_POST["checkbox-2"])){
	
		//Execute script on Nodo
		HTTPRequest("http://$nodo_ip/?id=$nodo_id&password=$nodo_password&event=FileExecute%20$file");
		
	}
	
	
	
	header("Location: scripts.php#saved");
	
	
}
/************************************************************************************************
END Script write													
*************************************************************************************************/
?>




<!DOCTYPE html> 
<html> 
 
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1"> 
	<title><?php echo $title ?></title> 
	<?php require_once('../include/jquery_mobile.php'); ?>
</head> 
 
<body> 

<!-- Start of main page: -->

<div data-role="page" pageid="main">
 
	<?php require_once('../include/header_admin.php'); ?>
 
	<div data-role="content">	

	<form action="scripts.php" data-ajax="false" method="post"> 
	
	 
				
	<br>
	
		<label for="select-script" class="select">Choose script:</label>
		<select name="select-script-1" id="select-script-1">
		<option value="EVENTLST" <?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "EVENTLST") {echo 'Selected="Selected"';}}?>>Eventlist</option>
		<option value="SCRIPT_1"<?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "SCRIPT_1") {echo 'Selected="Selected"';}}?>>Script 1</option>
		<option value="SCRIPT_2"<?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "SCRIPT_2") {echo 'Selected="Selected"';}}?>>Script 2</option>
		<option value="SCRIPT_3"<?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "SCRIPT_3") {echo 'Selected="Selected"';}}?>>Script 3</option>
		<option value="SCRIPT_4"<?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "SCRIPT_4") {echo 'Selected="Selected"';}}?>>Script 4</option>
		<option value="SCRIPT_5"<?php if (isset($_POST['select-script-1'])) {if ($_POST['select-script-1'] == "SCRIPT_5") {echo 'Selected="Selected"';}}?>>Script 5</option>
		</select>
		
		
		<input type="submit" name="Read" value="Read" >
		

		
		
		
		<label for="script">Script:</label>
		<textarea name="script" id="script"><?php 
		
		

if (isset($script)){  

	for($i=0;$i<$total_script_lines;$i++){
		
			
			//<br /> aan het einde van de regels verwijderen
			$script[$i] = str_replace("<br />","",$script[$i]);
			
			echo $script[$i];
 
		
	}
}

?>
</textarea>
<br>		
		
    	
		<input type="checkbox" name="checkbox-2" id="checkbox-2" class="custom" checked="yes"/>
		<label for="checkbox-2">Execute after write</label>
		
		<br>
        
		<input type="submit" name="Write" value="Write" > 
			

	
	</form> 
	
	
	
		
	
	</div><!-- /content -->
	
	<?php require_once('../include/footer_admin.php'); ?>
	
</div><!-- /main page -->

<!-- Start of saved page: -->
<div data-role="dialog" id="saved">

	<div data-role="header">
		<h1><?php echo $page_title?></h1>
	</div><!-- /header -->

	<div data-role="content">	
		<h2>Script send to Nodo.</h2>
				
		<p><a href="scripts.php" data-role="button" data-inline="true" data-icon="back">Ok</a></p>	
	
	
	</div><!-- /content -->
	
	
</div><!-- /page saved -->
 
</body>
</html>
