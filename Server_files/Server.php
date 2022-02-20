<?php

//api key
$api_key_value = "";

// REPLACE with your Database name
$database = "";
// REPLACE with Database user
$username = "";
// REPLACE with Database user password
$password = "";

// REPLACE with servername
$server = "";

$api_key = $value1 = $value2  = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") 
{
    $api_key = test($_POST["api_key"]);
    if($api_key == $api_key_value) 
    {
        $value1 = test($_POST["value1"]);
        $value2 = test($_POST["value2"]);
        
        
        // Create database connection
        $conn = new mysqli($server, $username, $password, $database);
        // Test connection
        if ($conn->connect_error) 
        {
            die("Connection failed: " . $conn->connect_error);
        } 
		
        //insert data to database
        $sql = "INSERT INTO Data1 (value1, value2)
        VALUES ('" . $value1 . "', '" . $value2 . "')";
        
        if ($conn->query($sql) === TRUE)
         {
            echo "OK";
        } 
        else 
        {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
    
        $conn->close();
    }
    else {
        echo "Wrong API Key.";
    }

}



function test($data) 
{  
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    $data = trim($data);
    return $data;
}

?>
</table>
</body>
</html>