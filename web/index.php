<?php
    $SQL = mysql_connect( 'localhost', '<>', '<>' );
    mysql_select_db( 'staylor_WiiSPACE', $SQL );
    $a = $_GET[ "a" ];
    if ( $a != "1p" && $a != "2p" && $a != "3p" && $a != "4p" && $a != "bm" && $a != "send" )
        $a = "";

    function serverCrypt( $s )
    {
        $k = "<>";
        $r = "";
        for ( $i = 0; $i < strlen( $s ); $i++ ) {
            $r .= chr( ord( substr( $s, $i, 1 ) ) ^ ord( substr( $k, $i % strlen( $k ), 1 ) ) );
        }
        return $r;
    }
        
    function display( $table, $name, $times )
    {
        global $SQL;
        $q = "SELECT * FROM `" . $table . "` ORDER BY `score` " . ( $times ? "ASC" : "DESC" ) . ", `name` ASC";
        $r = mysql_query( $q, $SQL );
        $n = mysql_num_rows( $r );
        
        echo "<br><br><br><div class='box'><b>" . $name . "</b><table class='big'>";
        for ( $i = 0; $i < 8 * ( $times ? 1 : 8 ); $i++ ) {
            if ( $i < $n )
                $s = mysql_result( $r, $i, 'score' );
            if ( !$s || $i >= $n ) {
                echo "<tr><td>----</td><td></td></tr>";
                continue;
            }
            if ( $times ) {
                $mins = 0;
                while ( $s >= 60 && $mins < 99 ) {
                    $s -= 60;
                    $mins++;
                }
                $secs = $s;
                $s = $mins . ":" . ( $secs < 10 ? "0" : "" ) . $secs;
            }
            echo "<tr><td class'big'>" . $s . "</td><td>" . mysql_result( $r, $i, 'name' ) . "</td></tr>";
        }
        echo "</table></div>";
    }
    
    function getTableName( $type )
    {
        $table = "";
        if ( $type == 0 )
            $table = "Scores1P";
        if ( $type == 1 )
            $table = "Scores2P";
        if ( $type == 2 )
            $table = "Scores3P";
        if ( $type == 3 )
            $table = "Scores4P";
        if ( $type == 4 )
            $table = "Boss1P";
        if ( $type == 5 )
            $table = "Boss2P";
        if ( $type == 6 )
            $table = "Boss3P";
        if ( $type == 7 )
            $table = "Boss4P";
        return $table;
    }
    
    if ( $a == "send" ) {
        $s = serverCrypt( stripslashes( $_GET[ "s" ] ) );
        $updates = array();
        
        $loop = 1;
        while ( $loop ) {
        
            $type = substr( $s, 0, 1 );
            $s = substr( $s, 2 );
            
            $p = strpos( $s, " " );
            $score = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            $p = strpos( $s, " " );
            $score17 = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            $p = strpos( $s, " " );
            $score701 = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            $p = strpos( $s, " " );
            $score1171 = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            $p = strpos( $s, " " );
            $score1777 = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            $p = strpos( $s, "#" );
            $name = substr( $s, 0, $p );
            $s = substr( $s, $p + 1 );
            
            if ( $type < 0 || $type >= 8 || !$score || $score % 17 != $score17 || $score % 701 != $score701 || $score % 1171 != $score1171 || $score % 1777 != $score1777 )
                exit( "invalid" );            
                
            $q = "SELECT * FROM `" . getTableName( $type ) . "` WHERE `name` = '" . $name . "' AND `score` = '" . $score . "'";
            $r = mysql_query( $q, $SQL );
            if ( mysql_num_rows( $r ) <= 0 ) {
            
                $updates[ $type ][] = array( $name, $score );
            
            }
            
            if ( substr( $s, 0, 1 ) == "#" ) {
                $s = substr( $s, 1 );
            }
            else {
                $loop = false;
            }
        
        }
        
        foreach ( $updates as $type => $paira ) {
            
        
            $q = "INSERT INTO `" . getTableName( $type ) . "` (`name`, `score`) VALUES ";
            $b = 0;
            foreach ( $paira as $i => $pair ) {
                $already = 0;
                for ( $j = $i - 1; $j >= 0; $j-- ) {
                    if ( $pair[ 0 ] == $paira[ j ][ 0 ] && $pair[ 1 ] == $paira[ j ][ 1 ] ) {
                        $already = 1;
                        $j = 0;
                    }
                }
                if ( $already )
                    continue;
                if ( $b )
                    $q .= ", ";
                $b = 1;
                    
                $q .= "('" . $pair[ 0 ] . "', '" . $pair[ 1 ] . "')";
            }
            mysql_query( $q );
            
            $q = "SELECT * FROM `" . getTableName( $type ) . "`";
            $n = mysql_num_rows( mysql_query( $q, $SQL ) ) - 8 * ( $type >= 4 ? 1 : 8 );
            if ( $n > 0 ) {
                $q = "DELETE FROM `" . getTableName( $type ) . "` ORDER BY `score` " . ( $type >= 4 ? "DESC" : "ASC" ) . " LIMIT " . $n;
                mysql_query( $q, $SQL );
            }
        }
        
        exit( "valid" );
    }
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
    <head>
    
        <title>
            WiiSPACE online high scores
        </title>
        <meta http-equiv='content-type' content='text/html; charset=UTF-8'>
        <link href='style.css' rel='stylesheet' type='text/css'>
        
    </head>
    
    <body>
        
        <div class='page'>
        
            <div class='box'>

                <b>WiiSPACE online high scores</b>
                <br><br>View high scores:
                <table>
                    <tr>
                        <?php
                            if ( $a != "1p" )
                                echo "<td><a href='?a=1p'>1 player</a></td>";
                            else
                                echo "<td><b>1 player</b></td>";
                            if ( $a != "2p" )
                                echo "<td><a href='?a=2p'>2 players</a></td>";
                            else
                                echo "<td><b>2 players</b></td>";
                            if ( $a != "3p" )
                                echo "<td><a href='?a=3p'>3 players</a></td>";
                            else
                                echo "<td><b>3 players</b></td>";
                            if ( $a != "4p" )
                                echo "<td><a href='?a=4p'>4 players</a></td>";
                            else
                                echo "<td><b>4 players</b></td>";
                            if ( $a != "bm" )
                                echo "<td><a href='?a=bm'>Boss mode</a></td>";
                            else
                                echo "<td><b>Boss mode</b></td>";
                        ?>
                    </tr>
                </table>
                
            </div>
            
            <?php
                $table = 0;
                
                if ( !$a ) {
                    echo "<br><br><br><div class='box'>Welcome to the online high score table for WiiSPACE. Visit the <a href='http://www.wiibrew.org/wiki/WiiSPACE'>wiki page</a> page for more information about the game.<br><br>";
                    echo "If your Wii is connected to the internet, WiiSPACE v1.2 or greater will upload your high scores on exit. However, for security reasons, high scores achieved before version 1.2 are not sent.</div>";
                }
                else if ( $a == "1p" )
                    display( "Scores1P", "High scores (1 player)", 0 );
                else if ( $a == "2p" )
                    display( "Scores2P", "High scores (2 players)", 0 );
                else if ( $a == "3p")
                    display( "Scores3P", "High scores (3 players)", 0 );
                else if ( $a == "4p" )
                    display( "Scores4P", "High scores (4 players)", 0 );
                else if ( $a == "bm" ) {
                    display( "Boss1P", "High scores (1 player boss mode)", 1 );
                    display( "Boss2P", "High scores (2 player boss mode)", 1 );
                    display( "Boss3P", "High scores (3 player boss mode)", 1 );
                    display( "Boss4P", "High scores (4 player boss mode)", 1 );                
                }
                
            ?>

        </div>
        
    </body>
</html>
<?php

    mysql_close( $SQL );

?>