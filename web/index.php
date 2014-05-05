<?

$BG_COUNT          = 23;
$DB_HOST           = "localhost";
$DB_USERNAME       = "<>";
$DB_PASSWORD       = "<>";
$DB_DATABASE       = "<>";
$CAPTCHA_PUBLIC    = "<>";
$CAPTCHA_PRIVATE   = "<>";
$ENCRYPTION_KEY    = "<>";
$SCORE_CGI_URL     = "http://seiken.co.uk/cgi-bin/wiiscore";
$PLAYERS_PER_PAGE  = 48;
$REPLAYS_PER_PAGE  = 24;
$COMMENTS_PER_PAGE = 12;

$ACTION      = isset( $_GET[ "a" ] ) ? $_GET[ "a" ] : "";
$PARAM       = isset( $_GET[ "p" ] ) ? $_GET[ "p" ] : "";
$ORDER       = isset( $_GET[ "o" ] ) ? intval( $_GET[ "o" ] ) : 0;
$PAGE        = isset( $_GET[ "q" ] ) ? intval( $_GET[ "q" ] ) : 0;
$FILE        = isset( $_GET[ "f" ] ) ? intval( $_GET[ "f" ] ) : 0;
$ERROR       = "";
$LOGGED_USER = array( "id" => 0 );

function date_str( $date_time, $short = false )
{
    return date( $short ? "d.m.y" : "j M Y", strtotime( $date_time ) );
}

function time_str( $date_time )
{
    $t = strtotime( $date_time );
    return date( "H:i", $t ) .  " <font class=\"dark\">on</font> " . date( "j M Y", $t );
}

function score_to_time( $score )
{
    if ( $score === 0 )
        return "--:--";
    $mins = 0;
    while ( $score >= 60 * 60 && $mins < 100 ) {
        $score -= 60 * 60;
        ++$mins;
    }
    if ( $mins >= 100 )
        return "--:--";
    $secs = intval( $score / 60 );
    return ( $mins < 10 ? "0" : "" ) . $mins . ":" . ( $secs < 10 ? "0" : "" ) . $secs;
}

function players_str( $players, $short = false )
{
    if ( $players === 1 )
        return $short ? "1P" : "1 PLAYER";
    if ( $players === 2 )
        return $short ? "2P" : "2 PLAYERS";
    if ( $players === 3 )
        return $short ? "3P" : "3 PLAYERS";
    if ( $players === 4 )
        return $short ? "4P" : "4 PLAYERS";
    return $short ? "?P" : "??? PLAYERS";
}

function mode_str( $mode, $short = false, $colour = false, $medium = false )
{
    $r = "";
    if ( $mode === "GAME" && $colour )
        $r .= "<font class=\"normal_mode\">";
    if ( $mode === "BOSS" && $colour )
        $r .= "<font class=\"boss_mode\">";
    if ( $mode === "HARD" && $colour )
        $r .= "<font class=\"hard_mode\">";
    if ( $mode === "FAST" && $colour )
        $r .= "<font class=\"fast_mode\">";
    if ( $mode === "GAME" )
        $r .= $medium ? "NORMAL" : ( $short ? "" : "NORMAL MODE" );
    else if ( $mode === "BOSS" )
        $r .= $short ? "BOSS" : "BOSS MODE";
    else if ( $mode === "HARD" )
        $r .= $short ? "HARD" : "HARD MODE";
    else if ( $mode === "FAST" )
        $r .= $short ? "FAST" : "FAST MODE";
    else if ( $mode === "WHAT" && $colour )
        $r .= $short ? "<font color=\"#ff0000\">W</font><font color=\"#ffff00\">-</font><font color=\"#00ff00\">H</font><font color=\"#00ffff\">A</font><font color=\"#0000ff\">T</font>" : "<font color=\"#ff0000\">W</font><font color=\"#ff7f00\">-</font><font color=\"#ffff00\">H</font><font color=\"#7fff00\">A</font><font color=\"#00ff00\">T</font> <font color=\"#00ff7f\">M</font><font color=\"#00ffff\">O</font><font color=\"#007fff\">D</font><font color=\"#0000ff\">E</font>";
    else if ( $mode === "WHAT" && !$colour )
        $r .= $short ? "W-HAT" : "W-HAT MODE";
    else
        $r .= $short ? "???" : "??? MODE";
    if ( $mode !== "WHAT" && $colour )
        $r .= "</font>";
    return $r;
}

function xor_crypt( $text, $key )
{
    $r = "";
    for ( $i = 0; $i < strlen( $text ); ++$i ) {
        $c = substr( $text, $i, 1 ) ^ substr( $key, $i % strlen( $key ), 1 );
        if ( substr( $text, $i, 1 ) === '\0' || $c == '\0' )
            $r += substr( $text, $i, 1 );
        else
            $r += $c;
    }
    return $r;
}

function background()
{
    $set = isset( $_SESSION[ "bg" ] );
    $bg = $set ? $_SESSION[ "bg" ] : "";
    $i = 0;
    for ( $x = 0; $x < 16; ++$x ) {
        for ( $y = 0; $y < 16; ++$y ) {
            $r = $set ? intval( substr( $bg, $i * 2, 2 ) ) : ( rand() % 2 == 0 ? rand() % $GLOBALS[ "BG_COUNT" ] : -( 1 + rand() % 9 ) );
            echo "<div class=\"bg\" style=\"left: " . ( $x * 640 - 320 ) . "px; top: " . ( $y * 480 - 240 ) . "px; background-image: url(bg/" . ( $r >= 0 ? $r : "_" . -( 1 + $r ) ) . ".png);\"></div>";
            ++$i;
            if ( !$set )
                $bg .= ( $r >= 0 && $r < 10 ? "0" : "" ) . $r;
        }
    }
    if ( !$set )
        $_SESSION[ "bg" ] = $bg;
}

function head( $logged )
{
    if ( $logged[ "id" ] === 0 ) {
    ?>

    <div class="header">
        <font class="shade"><a href="?">HOME</a></font> . <font class="shade"><a href="?a=register">REGiSTER</a></font> . <font class="shade"><a href="?a=login">LOGiN</a></font>
    </div>

    <?
    }
    else {
    ?>

    <div class="header">
        <font class="shade"><a href="?">HOME</a></font> . logged in as <font class="shade"><a href="?a=profile"><? echo $logged[ "name" ]; ?></a></font> . <font class="shade"><a href="?a=do_logout">LOGOUT</a></font>
    </div>

    <?
    }
}

function foot()
{
    ?>
    <div class="footer">
        <font class="shade"><a href="?a=scoreboard">Hi-SCORES</a></font> . <font class="shade"><a href="?a=player_list">PLAYER LiST</a></font> . <font class="shade"><a href="?a=replay&amp;p=r">RANDOM</a></font> . <font class="shade"><a href="mailto:wiispace@seiken.co.uk">EMAiL</a></font> . <font class="shade"><a href="http://grandfunkdynasty.com">MUSiC</a></font>
    </div>
    <?
}

function recaptcha_submit( $submit = "Submit!" )
{
    ?>

    <div id="recaptcha_widget" style="display: none">
        <div id="recaptcha_image"></div><div class="recaptcha_image"></div><br>
        <table><tr><td>
            <span class="recaptcha_only_if_image">Enter the words above:</span>
            <span class="recaptcha_only_if_audio">Enter the words you hear:</span>
        </td><td class="r">
            <input type="text" id="recaptcha_response_field" name="recaptcha_response_field">
        </td></tr><tr><td>
            <div><a href="javascript:Recaptcha.reload()">Get another CAPTCHA</a></div>
            <div class="recaptcha_only_if_image"><a href="javascript:Recaptcha.switch_type('audio')">Get an audio CAPTCHA</a></div>
            <div class="recaptcha_only_if_audio"><a href="javascript:Recaptcha.switch_type('image')">Get an image CAPTCHA</a></div>
        </td><td class="r">
            <input class="button" type="submit" value="<? echo $submit; ?>">
        </td></tr></table>
    </div>
    <script type="text/javascript" src="http://www.google.com/recaptcha/api/challenge?k=<? echo $GLOBALS[ "CAPTCHA_PUBLIC" ]; ?>"></script>
    <noscript>
            <iframe src="http://www.google.com/recaptcha/api/noscript?k=your_public_key" height="300" width="500" frameborder="0"></iframe><br>
            <textarea name="recaptcha_challenge_field" rows="3" cols="40"></textarea>
            <input type="hidden" name="recaptcha_response_field" value="manual_challenge">
    </noscript>

    <?
}

function nocaptcha_submit( $submit = "Submit!" )
{
    ?>

    <table><tr><td></td><td class="r">
        <input class="button" type="submit" value="<? echo $submit; ?>">
    </td></tr></table>

    <?
}

function recaptcha_valid()
{
    require_once( "recaptchalib.php" );
    if ( !isset( $_POST[ "recaptcha_challenge_field" ] ) || !isset( $_POST[ "recaptcha_response_field" ] ) )
        return false;
    $resp = recaptcha_check_answer( $GLOBALS[ "CAPTCHA_PRIVATE" ], $_SERVER[ "REMOTE_ADDR" ], $_POST[ "recaptcha_challenge_field" ], $_POST[ "recaptcha_response_field" ] );
    return $resp->is_valid;
}

function get_account( $db, &$error, $id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array( "id" => 0 );
    }
    if ( $id === 0 ) {
        $error = "No account with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    $s = $db->prepare( "SELECT name, reg_date, comments, score FROM accounts WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return array( "id" => 0 );
    }
    $s->bind_param( "i", $id );
    $s->execute();
    $s->bind_result( $name, $reg_date, $comments, $score );
    if ( !$s->fetch() ) {
        $error = "No account with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    $user = array( "id" => $id, "name" => $name, "comments" => $comments, "reg_date" => $reg_date, "score" => $score );
    while ( $s->fetch() );
    return $user;
}

function get_replay( $db, &$error, $id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array( "id" => 0 );
    }
    if ( $id === 0 ) {
        $error = "No replay with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    if ( $id === "r" ) {
        $s = $db->prepare( "SELECT id, seed, version, mode, players, score, tool_assisted, team_name, replay, date, comments, downloads, uploader FROM replays ORDER BY RAND() LIMIT 1" );
        if ( !$s ) {
            $error = $db->error;
            return array( "id" => 0 );
        }
    }
    else {
        $s = $db->prepare( "SELECT id, seed, version, mode, players, score, tool_assisted, team_name, replay, date, comments, downloads, uploader FROM replays WHERE id = ? LIMIT 1" );
        if ( !$s ) {
            $error = $db->error;
            return array( "id" => 0 );
        }
        $s->bind_param( "i", $id );
    }
    $s->execute();
    $s->bind_result( $id, $seed, $version, $mode, $players, $score, $tool_assisted, $team_name, $replay, $date, $comments, $downloads, $uploader );
    if ( !$s->fetch() ) {
        $error = "No replay with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    $replay = array( "id" => $id, "seed" => $seed, "version" => $version, "mode" => $mode, "players" => $players, "score" => $score, "tool_assisted" => $tool_assisted, "team_name" => $team_name, "replay" => $replay, "date" => $date, "comments" => $comments, "downloads" => $downloads, "uploader" => $uploader );
    while ( $s->fetch() );
    return $replay;
}

function get_comment( $db, &$error, $id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array( "id" => 0 );
    }
    if ( $id === 0 ) {
        $error = "No comment with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    $s = $db->prepare( "SELECT author, replay, date, comments FROM comments WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return array( "id" => 0 );
    }
    $s->bind_param( "i", $id );
    $s->execute();
    $s->bind_result( $author, $replay, $date, $comments );
    if ( !$s->fetch() ) {
        $error = "No comment with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
    $user = array( "id" => $id, "author" => $author, "replay" => $replay, "date" => $date, "comments" => $comments );
    while ( $s->fetch() );
    return $user;
}

function count_replays_by_account( $db, &$error, $acc_id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return 0;
    }
    if ( $acc_id === 0 )
        return 0;

    $s = $db->prepare( "SELECT COUNT(id) FROM replays WHERE uploader = ?" );
    if ( !$s ) {
        $error = $db->error;
        return 0;
    }
    $s->bind_param( "i", $acc_id );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() )
        return 0;
    $count = $temp;
    while ( $s->fetch() );
    return $count;
}

function get_replays_by_account( $db, &$error, $acc_id, $low, $num )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array();
    }
    if ( $acc_id === 0 ) {
        $error = "No account with that ID exists. Please try again.";
        return array();
    }

    $s = $db->prepare( "SELECT replays.id, replays.version, replays.mode, replays.players, replays.score, replays.tool_assisted, replays.team_name, replays.replay, replays.date, replays.comments, replays.uploader, accounts.name FROM replays, accounts WHERE replays.uploader = ? AND replays.uploader = accounts.id ORDER BY replays.date DESC LIMIT ?, ?" );
    if ( !$s ) {
        $error = $db->error;
        return array();
    }
    $s->bind_param( "iii", $acc_id, $low, $num );
    $s->execute();
    $s->bind_result( $id, $version, $mode, $players, $score, $tool_assisted, $team_name, $replay, $date, $comments, $uploader, $uploader_name );
    $replays = array();
    $i = 0;
    while ( $s->fetch() )
        $replays[ $i++ ] = array( "id" => $id, "version" => $version, "mode" => $mode, "players" => $players, "score" => $score, "tool_assisted" => $tool_assisted, "team_name" => $team_name, "replay" => $replay, "date" => $date, "comments" => $comments, "uploader" => $uploader, "uploader_name" => $uploader_name );
    return $replays;
}

function count_replays_by_mode( $db, &$error, $mode, $players )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array();
    }
    if ( $players < 0 || $players > 4 )
        return 0;
        
    if ( $mode === "ALL" && $players === 0 )
        $where_str = "mode != 'BOSS'";
    else if ( $mode === "ALL" )
        $where_str = "mode != 'BOSS' AND players = ?";
    else if ( $players === 0 )
        $where_str = "mode = ?";
    else
        $where_str = "mode = ? AND players = ?";

    $s = $db->prepare( "SELECT COUNT(id) FROM replays WHERE " . $where_str );
    if ( !$s ) {
        $error = $db->error;
        return 0;
    }
    if ( $mode === "ALL" && $players === 0 )
        ;
    else if ( $mode === "ALL" )
        $s->bind_param( "i", $players );
    else if ( $players === 0 )
        $s->bind_param( "s", $mode );
    else
        $s->bind_param( "si", $mode, $players );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() )
        return 0;
    $count = $temp;
    while ( $s->fetch() );
    return $count;
}

function get_replays_by_mode( $db, &$error, $mode, $players, $order, $low, $num )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array();
    }
    if ( $players < 0 || $players > 4 )
        return array();
        
    if ( ( $order === 1 && $mode !== "BOSS" ) || ( $order === 0 && $mode === "BOSS" ) )
        $order_str = "replays.score ASC";
    else if ( $order === 2 )
        $order_str = "replays.date DESC";
    else if ( $order === 3 )
        $order_str = "replays.date ASC";
    else
        $order_str = "replays.score DESC";
        
    if ( $mode === "ALL" && $players === 0 )
        $where_str = "replays.mode != 'BOSS'";
    else if ( $mode === "ALL" )
        $where_str = "replays.mode != 'BOSS' AND replays.players = ?";
    else if ( $players === 0 )
        $where_str = "replays.mode = ?";
    else
        $where_str = "replays.mode = ? AND replays.players = ?";

    $s = $db->prepare( "SELECT replays.id, replays.version, replays.mode, replays.players, replays.score, replays.tool_assisted, replays.team_name, replays.replay, replays.date, replays.comments, replays.uploader, accounts.name FROM replays, accounts WHERE " . $where_str . " AND replays.uploader = accounts.id ORDER BY " . $order_str . " LIMIT ?, ?" );
    if ( !$s ) {
        $error = $db->error;
        return array();
    }
    if ( $mode === "ALL" && $players === 0 )
        $s->bind_param( "ii", $low, $num );
    else if ( $mode === "ALL" )
        $s->bind_param( "iii", $players, $low, $num );
    else if ( $players === 0 )
        $s->bind_param( "sii", $mode, $low, $num );
    else
        $s->bind_param( "siii", $mode, $players, $low, $num );
    $s->execute();
    $s->bind_result( $id, $version, $mode, $players, $score, $tool_assisted, $team_name, $replay, $date, $comments, $uploader, $uploader_name );
    $replays = array();
    $i = 0;
    while ( $s->fetch() )
        $replays[ $i++ ] = array( "id" => $id, "version" => $version, "mode" => $mode, "players" => $players, "score" => $score, "tool_assisted" => $tool_assisted, "team_name" => $team_name, "replay" => $replay, "date" => $date, "comments" => $comments, "uploader" => $uploader, "uploader_name" => $uploader_name );
    return $replays;
}

function get_cumulative_score_by_account( $db, &$error, $acc_id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array( "id" => 0 );
    }
    if ( $acc_id === 0 ) {
        $error = "No account with that ID exists. Please try again.";
        return array( "id" => 0 );
    }
        
    $s = $db->prepare( "SELECT score FROM replays WHERE mode = ? AND players = ? AND uploader = ? ORDER BY score DESC LIMIT 1" );
    $cumulative_score = 0;
    for ( $p = 1; $p <= 4; ++$p ) {
        for ( $m = 0; $m < 4; ++$m ) {
            if ( $m === 0 )
                $mode_str = "GAME";
            else if ( $m === 1 )
                $mode_str = "HARD";
            else if ( $m === 2 )
                $mode_str = "FAST";
            else
                $mode_str = "WHAT";
            if ( !$s ) {
                $error = $db->error;
                return array();
            }

            $s->bind_param( "sii", $mode_str, $p, $acc_id );
            $s->execute();
            $s->bind_result( $score );
            if ( $s->fetch() )
                $cumulative_score += $score;
            while ( $s->fetch() );
        }
    }
    return $cumulative_score;
}

function get_replays_by_seed( $db, &$error, $seed )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array( "id" => 0 );
    }
    if ( $seed === "" ) {
        $error = "Invalid seed.";
        return array( "id" => 0 );
    }

    $s = $db->prepare( "SELECT id, version, mode, players, score, tool_assisted, team_name, replay, date, comments, uploader FROM replays WHERE seed = ?" );
    if ( !$s ) {
        $error = $db->error;
        return array( "id" => 0 );
    }
    $s->bind_param( "s", $seed );
    $s->execute();
    $s->bind_result( $id, $version, $mode, $players, $score, $tool_assisted, $team_name, $replay, $date, $comments, $uploader );
    $replays = array();
    $i = 0;
    while ( $s->fetch() )
        $replays[ $i++ ] = array( "id" => $id, "version" => $version, "mode" => $mode, "players" => $players, "score" => $score, "tool_assisted" => $tool_assisted, "team_name" => $team_name, "replay" => $replay, "date" => $date, "comments" => $comments, "uploader" => $uploader );
    return $replays;
}

function get_replay_ranking( $db, &$error, $replay )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return -1;
    }
     
    $s = $db->prepare( "SET @rownum := 0" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    $s->execute();
    $order = $replay[ "mode" ] === "BOSS" ? "score ASC" : "score DESC";
    $s = $db->prepare( "SELECT rank FROM (
                            SELECT @rownum := @rownum + 1 AS rank, id FROM replays WHERE mode = ? AND players = ? ORDER BY " . $order .
                       ") as result WHERE id = ?" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    
    $s->bind_param( "sii", $replay[ "mode" ], $replay[ "players" ], $replay[ "id" ] );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() ) {
        $error = "No replay with that ID exists. Please try again.";
        return -1;
    }
    
    $rank = $temp;
    while ( $s->fetch() );
    return $rank;
}

function get_account_ranking( $db, &$error, $account )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return -1;
    }
     
    $s = $db->prepare( "SET @rownum := 0" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    $s->execute();
    $s = $db->prepare( "SELECT rank FROM (
                            SELECT @rownum := @rownum + 1 AS rank, id FROM accounts ORDER BY score DESC
                        ) as result WHERE id = ?" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    
    $s->bind_param( "i", $account[ "id" ] );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() ) {
        $error = "No account with that ID exists. Please try again.";
        return -1;
    }
    
    $rank = $temp;
    while ( $s->fetch() );
    return $rank;
}

function get_comment_ranking( $db, &$error, $comment )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return -1;
    }
     
    $s = $db->prepare( "SET @rownum := 0" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    $s->execute();
    $s = $db->prepare( "SELECT rank FROM (
                            SELECT @rownum := @rownum + 1 AS rank, id FROM comments WHERE replay = ? ORDER BY date DESC
                        ) as result WHERE id = ?" );
    if ( !$s ) {
        $error = $db->error;
        return -1;
    }
    
    $s->bind_param( "ii", $comment[ "replay" ], $comment[ "id" ] );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() ) {
        $error = "No comment with that ID exists. Please try again.";
        return -1;
    }
    
    $rank = $temp;
    while ( $s->fetch() );
    return $rank;
}

function count_comments_by_replay( $db, &$error, $rep_id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return 0;
    }
    if ( $rep_id === 0 )
        return 0;

    $s = $db->prepare( "SELECT COUNT(id) FROM comments WHERE replay = ?" );
    if ( !$s ) {
        $error = $db->error;
        return 0;
    }
    $s->bind_param( "i", $rep_id );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() ) {
        return 0;
    }
    $count = $temp;
    while ( $s->fetch() );
    return $count;
}

function get_comments_by_replay( $db, &$error, $rep_id, $low, $num )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array();
    }
    if ( $rep_id === 0 ) {
        $error = "No replay with that ID exists. Please try again.";
        return array();
    }

    $s = $db->prepare( "SELECT comments.id, comments.author, comments.date, comments.comments, accounts.name FROM comments, accounts WHERE comments.replay = ? AND comments.author = accounts.id ORDER BY comments.date ASC LIMIT ?, ?" );
    if ( !$s ) {
        $error = $db->error;
        return array();
    }
    $s->bind_param( "iii", $rep_id, $low, $num );
    $s->execute();
    $s->bind_result( $id, $author, $date, $text, $author_name );
    $comments = array();
    $i = 0;
    while ( $s->fetch() )
        $comments[ $i++ ] = array( "id" => $id, "author" => $author, "date" => $date, "comments" => $text, "author_name" => $author_name );
    return $comments;
}

function count_comments_by_account( $db, &$error, $acc_id )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return 0;
    }
    if ( $acc_id === 0 )
        return 0;

    $s = $db->prepare( "SELECT COUNT(id) FROM comments WHERE author = ?" );
    if ( !$s ) {
        $error = $db->error;
        return 0;
    }
    $s->bind_param( "i", $acc_id );
    $s->execute();
    $s->bind_result( $temp );
    if ( !$s->fetch() ) {
        return 0;
    }
    $count = $temp;
    while ( $s->fetch() );
    return $count;
}

function get_comments_by_account( $db, &$error, $acc_id, $low, $num )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return array();
    }
    if ( $acc_id === 0 ) {
        $error = "No account with that ID exists. Please try again.";
        return array();
    }

    $s = $db->prepare( "SELECT comments.id, comments.replay, comments.date, comments.comments, replays.team_name FROM comments, replays WHERE comments.author = ? AND comments.replay = replays.id ORDER BY comments.date DESC LIMIT ?, ?" );
    if ( !$s ) {
        $error = $db->error;
        return array();
    }
    $s->bind_param( "iii", $acc_id, $low, $num );
    $s->execute();
    $s->bind_result( $id, $replay, $date, $text, $replay_name );
    $comments = array();
    $i = 0;
    while ( $s->fetch() )
        $comments[ $i++ ] = array( "id" => $id, "replay" => $replay, "date" => $date, "comments" => $text, "replay_name" => $replay_name );
    return $comments;
}

function is_mode_unlocked( $db, &$error, $mode )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return false;
    }

    $s = $db->prepare( "SELECT COUNT(id) FROM replays WHERE mode = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return false;
    }
    $s->bind_param( "s", $mode );
    $s->execute();
    $s->bind_result( $count );
    $r = false;
    if ( $s->fetch() && $count > 0 )
        $r = true;
    while ( $s->fetch() );
    return $r;
}

function replay_list( $replay_list, $nums = false, $low = 0 )
{
    ?><table><?
    for ( $i = 0; $i < count( $replay_list ); ++$i ) {
        $t = $replay_list[ $i ][ "uploader_name" ];
        $name = "";
        for ( $j = 0; $j < strlen( $t ); ++$j ) {
            $name .= substr( $t, $j, 1 ) . "&#8203;";
        }
        ?><tr id="replay_<? echo $replay_list[ $i ][ "id" ]; ?>">
        <td class="auto"><? echo ( $nums ? "#" . ( $low + $i + 1 ) . " " : "" ) . "<a href=\"?a=replay&amp;p=" . $replay_list[ $i ][ "id" ] . "\">" . $replay_list[ $i ][ "team_name" ] . "</a>"; ?></td>
        <td class="auto"><? echo players_str( $replay_list[ $i ][ "players" ], true ) . " " . mode_str( $replay_list[ $i ][ "mode" ], true, true ); ?></td>
        <td class="autor"><? echo $replay_list[ $i ][ "mode" ] === "BOSS" ? score_to_time( $replay_list[ $i ][ "score" ] ) : $replay_list[ $i ][ "score" ]; ?></td>
        </tr><tr>
        <td class="auto"></td><td class="auto"><font class="dark"><? echo date_str( $replay_list[ $i ][ "date" ], true ); ?></font></td><td class="autor"><font class="dark">(<a href="?a=profile&amp;p=<? echo $replay_list[ $i ][ "uploader" ]; ?>"><? echo $name; ?></a>)</font></td>
        </tr><?
    }
    ?></table><?
}

function pages( $text, $url, $pages, $reverse = false )
{
    static $id = 0;
    ?>
    <div class="maindark" id="pages_<? echo $id . "\">";
        echo $text;
        for ( $i = $reverse ? $pages - 1 : 0; $reverse ? $i >= 0 : $i < $pages; $reverse ? --$i : ++$i ) {
            $show = $i < 2 || abs( $i - $GLOBALS[ "PAGE" ] ) < 3 || $pages - 1 - $i < 2;
            $j = $reverse ? $i - 1 : $i + 1;
            $next = $j < 2 || abs( $j - $GLOBALS[ "PAGE" ] ) < 3 || $pages - 1 - $j < 2;
            if ( $show ) {
                echo $i !== $GLOBALS[ "PAGE" ] ? "<a href=\"" . $url . "&amp;q=" . $i . "\">" . ( $reverse ? $pages - $i : $i + 1 ) . "</a>" : ( $reverse ? $pages - $i : $i + 1 );
                if ( $reverse ? $i >= 1 : $i < $pages - 1 ) {
                    echo " ";
                    if ( !$next )
                        echo "<font class=\"dark\">...</font> ";
                }
            }
        }
    ?></div>
    <?
    ++$id;
}

function home_page( $error, $logged )
{
    ?>
    <div class="head">
        <font color="#ff0000">W</font><font color="#ff5500">i</font><font color="#ffaa00">i</font><font color="#ffff00">S</font><font color="#ffff00">P</font><font color="#ffaa00">A</font><font color="#ff5500">C</font><font color="#ff0000">E</font>
    </div>
    <div class="first">
        <? if ( $error !== "" ) echo "<font class=\"error\">" . $error . "</font><br><br>"; ?>
        WiiSPACE was originally a homebrew Wii game.
        (<a href="http://wiibrew.org/wiki/WiiSPACE">See here.</a>) Now there's an all-new PC version with many improvements.
        <br><br>
        <a href="http://www.youtube.com/watch?v=yWDvN6CHsds"><font color="#ffffff">That's is a puzzle!</font></a>
        <br><br>
        <font class="score">Download</font> <font class="dark">(Version 1.3 RC1)</font><br>
        <a href="?f=1">Windows (32-bit)</a> <font class="dark">.</font> <a href="?f=3">Linux (32-bit)</a><br>
        <a href="?f=2">Windows (64-bit)</a> <font class="dark">.</font> <a href="?f=4">Linux (64-bit)</a><br><br>
        Windows version requires <a href="http://www.microsoft.com/en-us/download/details.aspx?id=4190">XInput driver</a> (also comes with DirectX!).<br>
        <font class="dark">This is a release candidate version. If significant problems arise, possibly I'll fix them and wipe the database...</font>
        <br><br>
        <a href="?a=scoreboard">HiGH SCORES & REPLAYS! CLICK HERE!</a>
        <? if ( $logged[ "id" ] === 0 ) echo "<a href=\"?a=login\">Login</a> or <a href=\"?a=register\">register</a> to upload your replays!<br>"; ?>
        <br><br>
        Below: a special video from the current top scorer...<br>
        <iframe class="video" src="http://www.youtube.com/embed/aRZlXM7Gy8k" frameborder="0" allowfullscreen></iframe><div class="video"></div>
    </div>
    <?
}

function do_static_file( $db, &$error, $logged, $param )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return;
    }
    
    $s = $db->prepare( "SELECT file, downloads FROM static_files WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return;
    }
    $s->bind_param( "i", $param );
    $s->execute();
    $s->bind_result( $a, $b );
    if ( !$s->fetch() ) {
        $error = "No file exists with that ID. Please try again.";
        return;
    }
    $file = $a;
    $downloads = $b;
    while( $s->fetch() );
    header( "Location: " . $file );
    $s = $db->prepare( "UPDATE static_files SET downloads = ? WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return;
    }
    $d = $downloads + 1;
    $s->bind_param( "ii", $d, $param );
    $s->execute();
    return;
}

function register_form( $error )
{
    require_once( "recaptchalib.php" );
    ?>

    <div class="head">
        Register
    </div>
    <div class="first">
    <? if ( $error !== "" ) echo "<font class=\"error\">" . $error . "</font><br><br>"; ?>
    Registering an account will let you upload replays and comment on other players'.<br><br>
    <form method="post" action="?a=do_register">
    <table><tr><td>
        Account name:
    </td><td class="r">
        <input type="text" name="acc_name" id="acc_name" value="<? if ( isset( $_POST[ "acc_name" ] ) ) echo htmlspecialchars( $_POST[ "acc_name" ] ); ?>" maxlength="20">
    </td></tr><tr><td>
       Account password:
    </td><td class="r">
        <input type="password" id="acc_pass" name="acc_pass">
    </td></tr><tr><td>
       <font class="dark">Repeat password:</font>
    </td><td class="r">
        <input type="password" id="acc_pass_repeat" name="acc_pass_repeat">
    </td></tr></table>
    <br>
    <? recaptcha_submit(); ?>
    </form>
    </div>

    <?
}

function do_register( $db, &$error, &$logged, &$param )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "register";
    }
    $acc_name = isset( $_POST[ "acc_name" ] ) ? $_POST[ "acc_name" ] : "";
    $acc_pass = isset( $_POST[ "acc_pass" ] ) ? $_POST[ "acc_pass" ] : "";
    $acc_pass_repeat = isset( $_POST[ "acc_pass_repeat" ] ) ? $_POST[ "acc_pass_repeat" ] : "";
    $captcha_pass = recaptcha_valid();

    if ( strlen( $acc_name ) < 2 || strlen( $acc_name ) > 20 ) {
        $error = "Please choose a name between 2 and 20 characters long.";
        return "register";
    }

    if ( !ctype_alnum( $acc_name ) ) {
        $error = "Please use only alphanumeric characters in your account name.";
        return "register";
    }

    $s = $db->prepare( "SELECT COUNT(id) FROM accounts WHERE name = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return "register";
    }
    $s->bind_param( "s", $acc_name );
    $s->execute();
    $s->bind_result( $count );
    $s->fetch();
    if ( $count > 0 ) {
        $error = "That name is already taken. Please try another.";
        return "register";
    }
    while ( $s->fetch() );

    if ( strlen( $acc_pass ) < 6 || strlen( $acc_pass_repeat ) < 6 ) {
        $error = "Please choose a password 6 or more characters long.";
        return "register";
    }

    if ( $acc_pass !== $acc_pass_repeat ) {
        $error = "The passwords didn't match. Please try again.";
        return "register";
    }

    if ( !$captcha_pass ) {
        $error = "Incorrect CAPTCHA response! Please try again.";
        return "register";
    }

    $salt = substr( md5( uniqid( rand(), true ) ), 0, 16 );
    $comments = "";
    $s = $db->prepare( "INSERT INTO accounts (name, hash, salt, reg_date, comments) VALUES (?, ?, ?, NOW(), ?)" );
    if ( !$s ) {
        $error = $db->error;
        return "register";
    }
    $hash = sha1( $salt . $acc_pass );
    $s->bind_param( "ssss", $acc_name, $hash, $salt, $comments );
    $s->execute();
    return do_login( $db, $error, $logged, $param );
}

function login_form( $error )
{
    ?>

    <div class="head">
        Login
    </div>
    <div class="first">
    <? if ( $error !== "" ) echo "<font class=\"error\">" . $error . "</font><br><br>"; ?>
    <form method="post" action="?a=do_login">
    <table><tr><td>
        Account name:
    </td><td class="r">
        <input type="text" name="acc_name" id="acc_name" value="<? if ( isset( $_POST[ "acc_name" ] ) ) echo htmlspecialchars( $_POST[ "acc_name" ] ); ?>" maxlength="20">
    </td></tr><tr><td>
       Account password:
    </td><td class="r">
        <input type="password" id="acc_pass" name="acc_pass">
    </td></tr></table>
    <br>
    <? nocaptcha_submit( "Login!" ); ?>
    </form>
    </div>

    <?
}

function do_login( $db, &$error, &$logged, &$param )
{
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "login";
    }
    $acc_name = isset( $_POST[ "acc_name" ] ) ? $_POST[ "acc_name" ] : "";
    $acc_pass = isset( $_POST[ "acc_pass" ] ) ? $_POST[ "acc_pass" ] : "";

    $s = $db->prepare( "SELECT id, hash, salt FROM accounts WHERE name = ?" );
    if ( !$s ) {
        $error = $db->error;
        return "login";
    }
    $s->bind_param( "s", $acc_name );
    $s->execute();
    $s->bind_result( $id, $hash, $salt );

    $exist = false;
    $valid_id = 0;
    while ( $s->fetch() ) {
        $exist = true;
        if ( sha1( $salt . $acc_pass ) === $hash )
            $valid_id = $id;
    }
    
    if ( !$exist ) {
        $error = "No account with that name exists. Please try again.";
        return "login";
    }
    if ( $valid_id === 0 ) {
        $error = "Incorrect password. Please try again.";
        return "login";
    }

    session_regenerate_id();
    $_SESSION[ "id" ] = $valid_id;
    unset( $_SESSION[ "bg" ] );
    $logged = get_account( $db, $error, $valid_id );
    $param = strval( $valid_id );
    header( "Location: ?a=profile" );
    return "profile";
}

function do_logout( &$logged )
{
    session_destroy();
    $logged = array( "id" => 0 );
    session_start();
    header( "Location: ?" );
    return "home";
}

function profile( $db, $error, $logged, $view, $edit )
{
    $rank = get_account_ranking( $db, $error, $view );
    ?>

    <div class="head">
        Profile
    </div>
    <div class="first">
    <?
    if ( $view[ "id" ] === 0 )
         $error = "No account with that ID exists. Please try again.";
    if ( $error !== "" )
        echo "<font class=\"error\">" . $error . "</font>";
    else if ( $logged[ "id" ] === $view[ "id" ] && $edit ) {
        ?>
        
        <font class="dark">Profile for</font> <? echo $view[ "name" ]; ?><font class="dark">.</font><br><br>
        <form method="post" action="?a=do_edit_profile">
        About:
        <textarea name="comments" id="comments"><? echo isset( $_POST[ "comments" ] ) ? htmlspecialchars( $_POST[ "comments" ] ) : htmlspecialchars( $view[ "comments" ] ); ?></textarea><br><br>
        <? nocaptcha_submit( "Save!" ); ?>
        </form>
            
        <?
    }
    else {
        ?>

        <div class="score">
            <font class="dark">Total score:</font><br><font class="score"><? echo $view[ "score" ]; ?></font>
            <? if ( $rank >= 0 ) echo "<br><font class=\"dark\">Rank:</font> <font class=\"score\">#" . $rank . "</font> (<a href=\"?a=player_list&amp;o=0&amp;q=" . intval( ( $rank - 1 ) / $GLOBALS[ "PLAYERS_PER_PAGE" ] ) . "#player_" . $view[ "id" ] . "\">view</a>)"; ?>
        </div>
        <font class="dark">Profile for</font> <? echo $view[ "name" ]; ?><font class="dark">.</font><br><br>
        About<? if ( $logged[ "id" ] === $view[ "id" ] ) echo " (<a href=\"?a=edit_profile\">edit</a>)"; ?>:
        <br><font class="comments">
        <? echo $view[ "comments" ] === "" ? "<font class=\"dark\">(none)</font>" : nl2br( htmlspecialchars( $view[ "comments" ] ) ); ?>
        </font>
        <br><br>
        <font class="dark">Registered since</font> <? echo date_str( $view[ "reg_date" ] ); ?><font class="dark">.</font><br>
        <a href="?a=profile_comments&p=<? echo $view[ "id" ]; ?>">All comments by <? echo $view[ "name" ]; ?></a>
        <?
    }
    ?>
    </div>
    <?
    if ( $error === "" && isset( $view[ "id" ] ) && $view[ "id" ] > 0 && !$edit ) {
        $replays_count = count_replays_by_account( $db, $error, $view[ "id" ] );
        $pages = 1 + intval( ( $replays_count - 1 ) / $GLOBALS[ "REPLAYS_PER_PAGE" ] );
        $GLOBALS[ "PAGE" ] = max( 0, min( $pages - 1, $GLOBALS[ "PAGE" ] ) );
        $low = $GLOBALS[ "PAGE" ] * $GLOBALS[ "REPLAYS_PER_PAGE" ];
        if ( $pages >= 2 )
            pages( $replays_count . " <font class=\"dark\">replays:</font> ", "?a=profile&amp;p=" . $view[ "id" ], $pages );
            
        $error = "";
        $replay_list = get_replays_by_account( $db, $error, $view[ "id" ], $low, $GLOBALS[ "REPLAYS_PER_PAGE" ] );
        ?>
        <div class="main">
        Replays by <? echo $view[ "name" ]; if ( $logged[ "id" ] === $view[ "id" ] ) echo " (<a href=\"?a=upload\">upload</a>)"; ?>:<br><br>
        <?
        if ( $error !== "" )
            echo "<font class=\"error\">" . $error . "</font>";
        else if ( count( $replay_list ) === 0 )
            echo "<font class=\"dark\">(none)</font>";
        else
            replay_list( $replay_list );
        ?>
        </div>
        
        <?
        if ( $pages >= 2 )
            pages( $replays_count . " <font class=\"dark\">replays:</font> ", "?a=profile&amp;p=" . $view[ "id" ], $pages );
    }
}

function profile_comments( $db, $error, $logged, $view )
{
    $comments_count = count_comments_by_account( $db, $error, $view[ "id" ] );
    $pages = 1 + intval( ( $comments_count - 1 ) / $GLOBALS[ "COMMENTS_PER_PAGE" ] );
    $GLOBALS[ "PAGE" ] = max( 0, min( $pages - 1, $GLOBALS[ "PAGE" ] ) );
    $low = $GLOBALS[ "PAGE" ] * $GLOBALS[ "COMMENTS_PER_PAGE" ];
    if ( $pages >= 2 )
        pages( $comments_count . " <font class=\"dark\">comments:</font> ", "?a=profile_comments&amp;p=" . $view[ "id" ], $pages );

    $comments_list = get_comments_by_account( $db, $error, $view[ "id" ], $low, $GLOBALS[ "COMMENTS_PER_PAGE" ] );
    if ( $error !== "" ) {
        echo "<div class=\"main\"><font class=\"error\">" . $error . "</font></div>";
        return;
    }
    if ( count( $comments_list ) === 0 ) {
        echo "<div class=\"main\"><font class=\"dark\">No comments found for <a href=\"?a=profile&amp;p=" . $view[ "id" ] . "\">" . $view[ "name" ] . "</a>.</font></div>";
        return;
   } 
    for ( $i = 0; $i < count( $comments_list ); ++$i ) {
        ?>
        <div class="<? echo $i % 2 == 0 ? "main" : "mainmid"; ?>" id="comment_<? echo $comments_list[ $i ][ "id" ] ?>">
            <a href="?a=profile&amp;p=<? echo $view[ "id" ]; ?>"><? echo $view[ "name" ]; ?></a>
            <font class="dark">wrote at</font> <? echo time_str( $comments_list[ $i ][ "date" ] ); ?>
            on <a href="?a=replay&amp;p=<? echo $comments_list[ $i ][ "replay" ]; ?>#comment_<? echo $comments_list[ $i ][ "id" ]; ?>"><? echo $comments_list[ $i ][ "replay_name" ]; ?></a><?
            if ( $logged[ "id" ] == $view[ "id" ] )
                echo " (<a href=\"?a=edit_comment&amp;p=" . $comments_list[ $i ][ "id" ] . "#edit_comment\">edit</a>)";

            ?><font class="dark">:</font>
            <br><br>
            <font class="comments"><? echo nl2br( htmlspecialchars( $comments_list[ $i ][ "comments" ] ) ); ?></font>
        </div>
        <?
    }
    if ( $pages >= 2 )
        pages( $comments_count . " <font class=\"dark\">comments:</font> ", "?a=profile_comments&amp;p=" . $view[ "id" ], $pages );
}

function do_edit_profile( $db, &$error, $logged, &$param )
{
    if ( $logged[ "id" ] === 0 ) {
        $error = "You don't have permission to do that.";
        return "home";
    }
        
    $param = $logged[ "id" ];
    if ( !isset( $_POST[ "comments" ] ) )
        return "profile";

    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "edit_profile";
    }

    $s = $db->prepare( "UPDATE accounts SET comments = ? WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return "edit_profile";
    }
    $comments = trim( $_POST[ "comments" ] );
    $s->bind_param( "si", $comments, $logged[ "id" ] );
    $s->execute();
    header( "Location: ?a=profile" );
    return "profile";
}

function player_list( $db, $error, $order )
{
    $players = array();
    $players_count = 0;
    $low = 0;
    $pages = 0;
    if ( !$db )
        $error = "Could not connect to the database. Please try again.";
    else {
        $s = $db->prepare( "SELECT COUNT(id) FROM accounts" );
        if ( !$s )
            $error = $db->error;
        else {
            $s->execute();
            $s->bind_result( $temp );
            if ( $s->fetch() ) {
                $players_count = $temp;
                $pages = 1 + intval( ( $players_count - 1 ) / $GLOBALS[ "PLAYERS_PER_PAGE" ] );
                $GLOBALS[ "PAGE" ] = max( 0, min( $pages - 1, $GLOBALS[ "PAGE" ] ) );
                $low = $GLOBALS[ "PAGE" ] * $GLOBALS[ "PLAYERS_PER_PAGE" ];
            }
            while ( $s->fetch() );
            $s = $order !== 0 ? $db->prepare( "SELECT id, name FROM accounts ORDER BY name ASC LIMIT ?, ?" ) : $db->prepare( "SELECT id, name FROM accounts ORDER BY score DESC LIMIT ?, ?" );
            if ( !$s )
                $error = $db->error;
            else {
                $s->bind_param( "ii", $low, $GLOBALS[ "PLAYERS_PER_PAGE" ] );
                $s->execute();
                $s->bind_result( $id, $name );
            }
        }
    }

    $i = 0;
    while ( isset( $s ) && $s->fetch() ) {
        $players[ $i++ ] = array( "id" => $id, "name" => $name );
    }
    ?>
    <div class="head">
        Players
    </div>
    <div class="first">
        <?
        if ( $error !== "" )
            echo "<font class=\"error\">" . $error . "</font><br><br>";
        else {
            if ( count( $players ) == 0 )
                echo "<font class=\"dark\">(none)</font>";
            else {
                ?>
                <font class="dark">Order by:</font><br>
                <? if ( $order === 0 ) echo "Score"; else echo "<a href=\"?a=player_list&amp;o=0\">Score</a>"; ?>
                <? if ( $order === 1 ) echo "Name"; else echo "<a href=\"?a=player_list&amp;o=1\">Name</a>"; ?>
                <?
                if ( $pages < 2 )
                    echo "<br><br>"; 
            }
            if ( $pages >= 2 ) {
                echo "</div>";
                pages( $players_count . " <font class=\"dark\">players:</font> ", "?a=player_list&amp;o=" . $order, $pages );
                echo "<div class=\"main\">";
            }
            for ( $i = 0; $i < count( $players ); ++$i )
                echo ( $i > 0 && $i % 2 == 0 ? "<br>" : "" ) . ( $i % 2 == 1 ? "<div class=\"score\" id=\"player_" . $players[ $i ][ "id" ] . "\">" : "<div class=\"left\" id=\"player_" . $players[ $i ][ "id" ] . "\">" ) . ( $order === 0 && $i % 2 == 0 ? "#" . ( $low + $i + 1 ) . " " : "" ) . "<a href=\"?a=profile&amp;p=" . $players[ $i ][ "id" ] . "\">" . $players[ $i ][ "name" ] . "</a>" . ( $order === 0 && $i % 2 == 1 ? " #" . ( $low + $i + 1 ) : "" ) . "</div>";
            echo "<br><br>";
        }
        ?>
    </div>
    <?
    if ( $pages >= 2 )
        pages( $players_count . " <font class=\"dark\">players:</font> ", "?a=player_list&amp;o=" . $order, $pages );
}

function upload_form( $error )
{
    require_once( "recaptchalib.php" );
    ?>

    <div class="head">
        Upload replay
    </div>
    <div class="first">
    <? if ( $error !== "" ) echo "<font class=\"error\">" . $error . "</font><br><br>"; ?>
    <form method="post" action="?a=do_upload" enctype="multipart/form-data">
    <table><tr><td>
        Replay file:
    </td><td class="r">
        <input type="file" class="file" name="replay_file" id="replay_file">
    </td></tr></table>
    <br>
    <table><tr><td>
        Name <font class="dark">/</font> team name<br><font class="dark">(for scoreboard)</font>:
    </td><td class="r">
        <input type="text" id="team_name" name="team_name" maxlength="17" value="<? echo isset( $_POST[ "team_name" ] ) ? htmlspecialchars( $_POST[ "team_name" ] ) : "" ?>">
    </td></tr></table>
    <br>
    Comments:
    <textarea name="comments" id="comments"><? echo isset( $_POST[ "comments" ] ) ? htmlspecialchars( $_POST[ "comments" ] ) : ""; ?></textarea><br>
    <br>
    <? nocaptcha_submit(); ?>
    </form>
    <br><font class="dark">Please be patient! It might take a little while to upload and process your replay.</font>
    </div>

    <?
}

function do_upload( $db, &$error, $logged, &$param )
{
    if ( !isset( $_FILES[ "replay_file" ] ) ) {
        $error = "Upload failed. Please try again.";
        return "upload";
    }
    if ( intval( $_FILES[ "replay_file" ][ "error" ] ) > 0 ) {
        $error = "Upload failed. Please try again. (Error " . $_FILES[ "replay_file" ][ "error" ] . ")";
        return "upload";
    }
    if ( $_FILES[ "replay_file" ][ "size" ] > 1024 * 1024 * 32 ) {
        $error = "Replay file too large (over 32 megabytes).";
        return "upload";
    }
    $name = $_FILES[ "replay_file" ][ "name" ];
    if ( substr( $name, strlen( $name ) - 4 ) !== ".wrp" ) {
        $error = "Please upload a file with the .wrp extension.";
        return "upload";
    }
    
    if ( !isset( $_POST[ "team_name" ] ) || strlen( $_POST[ "team_name" ] ) < 1 ) {
        $error = "Please choose a name / team name for display on the scoreboard (for example, `AAA').";
        return "upload";
    }
    
    if ( $logged[ "id" ] === 0 ) {
        $error = "You don't have permission to do that.";
        return "home";
    }
    
    $c = curl_init( $GLOBALS[ "SCORE_CGI_URL" ] . "?" . $_FILES[ "replay_file" ][ "tmp_name" ] );
    curl_setopt( $c, CURLOPT_HEADER, 0 );
    curl_setopt( $c, CURLOPT_RETURNTRANSFER, 1 );
    $data = preg_split ( '/$\R?^/m', curl_exec( $c ) );
    curl_close( $c );
    
    if ( $data[ 0 ] !== "WiiSPACE v1.3 replay" ) {
        $error = "File is not a WiiSPACE replay.";
        return "upload";
    }
    
    $version = "1.3";
    $tool_assisted = 0;
    $seed     = intval( $data[ 1 ] );
    $players  = intval( $data[ 2 ] );
    if ( intval( $data[ 3 ] ) )
        $mode = "BOSS";
    else if ( intval( $data[ 4 ] ) )
        $mode = "HARD";
    else if ( intval( $data[ 5 ] ) )
        $mode = "FAST";
    else if ( intval( $data[ 6 ] ) )
        $mode = "WHAT";
    else
        $mode = "GAME";
    $score    = intval( $data[ 7 ] );
    if ( $mode === "BOSS" && $score === 0 )
        $score = 360000;
    
    if ( $players < 1 || $players > 4 || $score < 0 ) {
        $error = "File is not a WiiSPACE replay.";
        return "upload";
    }
    
    $exist = get_replays_by_seed( $db, $no_error, $seed );
    for ( $i = 0; $i < count( $exist ); ++$i ) {
        if ( $exist[ $i ][ "id" ] !== 0 && $exist[ $i ][ "players" ] === $players &&
             $exist[ $i ][ "mode" ] === $mode && $exist[ $i ][ "score" ] === $score &&
             $exist[ $i ][ "version" ] === $version ) {
            $error = "That replay has already been uploaded! <a href=\"?a=replay&amp;p=" . $exist[ $i ][ "id" ] . "\">View it here.</a>";
            return "upload";
        }
    }
    
    $replay_file = $logged[ "name" ] . "_" . strval( $players ) . "p_" . mode_str( $mode, true ) . "_" . ereg_replace( "[^A-Za-z0-9 !]", "", $_POST[ "team_name" ] ) . "_" . strval( $score );
    $s = $db->prepare( "INSERT INTO replays (seed, version, mode, players, score, tool_assisted, team_name, replay, date, comments, downloads, uploader) VALUES (?, ?, ?, ?, ?, ?, ?, ?, NOW(), ?, ?, ?)" );
    if ( !$s ) {
        $error = $db->error;
        return "upload";
    }
    $downloads = 0;
    $comments = trim( $_POST[ "comments" ] );
    $s->bind_param( "sssiiisssii", $seed, $version, $mode, $players, $score, $tool_assisted, $_POST[ "team_name" ], $replay_file, $comments, $downloads, $logged[ "id" ] );
    $s->execute();
    mkdir( "replays/" . $seed );
    move_uploaded_file( $_FILES[ "replay_file" ][ "tmp_name" ], "replays/" . $seed . "/" . $replay_file . ".wrp" );
    
    $cumulative_score = get_cumulative_score_by_account( $db, $error, $logged[ "id" ] );
    $s = $db->prepare( "UPDATE accounts SET score = ? WHERE id = ? LIMIT 1" );
    $s->bind_param( "ii", $cumulative_score, $logged[ "id" ] );
    $s->execute();
    
    $param = 0;
    $exist = get_replays_by_seed( $db, $no_error, $seed );
    for ( $i = 0; $i < count( $exist ); ++$i ) {
        if ( $exist[ $i ][ "id" ] !== 0 && $exist[ $i ][ "players" ] === $players &&
             $exist[ $i ][ "mode" ] === $mode && $exist[ $i ][ "score" ] === $score &&
             $exist[ $i ][ "version" ] === $version ) {
            $param = $exist[ $i ][ "id" ];
        }
    }
    unset( $GLOBALS[ "_POST" ][ "comments" ] );
    header( "Location: ?a=replay&p=" . $param );
    return "replay";
}

function replay( $db, $error, $logged, $replay, $edit, $edit_comment = array( "id" => 0 ) )
{
    $rank = get_replay_ranking( $db, $error, $replay );
    if ( !$edit && $edit_comment[ "id" ] !== 0 ) {
        $comment_rank = get_comment_ranking( $db, $error, $edit_comment );
        $GLOBALS[ "PAGE" ] = intval( ( $comment_rank - 1 ) / $GLOBALS[ "COMMENTS_PER_PAGE" ] );
    }
    $mp = $replay[ "players" ] - 1;
    if ( $replay[ "mode" ] === "BOSS" )
        $mp += 5;
    if ( $replay[ "mode" ] === "HARD" )
        $mp += 10;
    if ( $replay[ "mode" ] === "FAST" )
        $mp += 15;
    if ( $replay[ "mode" ] === "WHAT" )
        $mp += 20;
    ?>

    <div class="head">
        Replay
    </div>
    <div class="first">
    <?
    if ( $replay[ "id" ] === 0 )
         $error = "No replay with that ID exists. Please try again.";
    if ( $error !== "" )
        echo "<font class=\"error\">" . $error . "</font>";
    else {
        $uploader = get_account( $db, $error, $replay[ "uploader" ] );
        ?>
        
        <? echo htmlspecialchars( $replay[ "team_name" ] ); ?>
        <div class="score">
            <font class="dark">Score:</font> <font class="score"><? echo $replay[ "mode" ] === "BOSS" ? score_to_time( $replay[ "score" ] ) : $replay[ "score" ]; ?></font><br>
            <a href="<? echo "?a=do_download&amp;p=" . $replay[ "id" ]; ?>">Download replay file!</a><br>
            v<? echo $replay[ "version" ]; ?> <font class="dark">(<? echo $replay[ "downloads" ]; ?> downloads.)</font>
            <? if ( $rank >= 0 ) echo "<br><font class=\"dark\">Rank:</font> <font class=\"score\">#" . $rank . "</font> (<a href=\"?a=scoreboard&amp;p=" . $mp . "&amp;o=0&amp;q=" . intval( ( $rank - 1 ) / $GLOBALS[ "REPLAYS_PER_PAGE" ] ) . "#replay_" . $replay[ "id" ] . "\">view</a>)"; ?>
        </div>
        <br>
        <font class="dark"><? echo mode_str( $replay[ "mode" ], false, true ) . "<br>" . players_str( $replay[ "players" ] ); ?>
        </font>
        <br><br>

        <?
        if ( $edit ) {
            ?>
            <form method="post" action="?a=do_edit_replay&amp;p=<? echo $replay[ "id" ]; ?>">
            Uploader's comments:
            <textarea name="comments" id="comments"><? echo isset( $_POST[ "comments" ] ) ? htmlspecialchars( $_POST[ "comments" ] ) : htmlspecialchars( $replay[ "comments" ] ); ?></textarea><br><br>
            <? nocaptcha_submit( "Save!" ); ?>
            </form>
            <?
        }
        else {
            ?>
            Uploader's comments<? if ( $logged[ "id" ] === $uploader[ "id" ] ) echo " (<a href=\"?a=edit_replay&amp;p=" . $replay[ "id" ] . "\">edit</a>)"; ?>:
            <br><font class="comments">
            <? echo $replay[ "comments" ] === "" ? "<font class=\"dark\">(none)</font>" : nl2br( htmlspecialchars( $replay[ "comments" ] ) ); ?>
            </font>

            <?
        }
        if ( !$edit ) {
            ?><br><br><font class="dark">Uploaded on</font> <? echo date_str( $replay[ "date" ] ) ?><font class="dark"> by <? echo "<a href=\"?a=profile&amp;p=" . $uploader[ "id" ] . "\">" . $uploader[ "name" ] . "</a>"; ?>.</font><?
        }
    }
    ?>
    </div>
    <?
    if ( $logged[ "id" ] !== 0 && $replay[ "id" ] !== 0 && !$edit && $edit_comment[ "id" ] === 0 ) {
        ?>
        <div class="main">
        <form method="post" action="?a=do_add_comment&amp;p=<? echo $replay[ "id" ]; ?>">
        Add comment:
        <textarea name="comments" id="comments"><? echo isset( $_POST[ "comments" ] ) ? htmlspecialchars( $_POST[ "comments" ] ) : ""; ?></textarea>
        <br><br>
        <? nocaptcha_submit( "Save!" ); ?>
        </form>
        </div>
        <?
    }
    else if ( $logged[ "id" ] !== 0 && !$edit && $edit_comment[ "id" ] !== 0 ) {
        ?>
        <div class="main" id="edit_comment">
        <form method="post" action="?a=do_edit_comment&amp;p=<? echo $edit_comment[ "id" ]; ?>">
        Edit comment <font class="dark">written at</font> <? echo time_str( $edit_comment[ "date" ] ); ?>:
        <textarea name="comments" id="comments"><? echo isset( $_POST[ "comments" ] ) ? htmlspecialchars( $_POST[ "comments" ] ) : htmlspecialchars( $edit_comment[ "comments" ] ); ?></textarea>
        <br><br>
        <? nocaptcha_submit( "Save!" ); ?>
        </form>
        </div>
        <?
    }
    if ( $replay[ "id" ] !== 0 ) {
        $error = "";
        $comments_count = count_comments_by_replay( $db, $error, $replay[ "id" ] );
        $pages = 1 + intval( ( $comments_count - 1 ) / $GLOBALS[ "COMMENTS_PER_PAGE" ] );
        $GLOBALS[ "PAGE" ] = max( 0, min( $pages - 1, $GLOBALS[ "PAGE" ] ) );
        $low = $comments_count - ( ( 1 + $GLOBALS[ "PAGE" ] ) * $GLOBALS[ "COMMENTS_PER_PAGE" ] );
        if ( $pages >= 2 )
            pages( $comments_count . " <font class=\"dark\">comments:</font> ", "?a=replay&amp;p=" . $replay[ "id" ], $pages, true );

        $comments_list = get_comments_by_replay( $db, $error, $replay[ "id" ], max( 0, $low ), $GLOBALS[ "COMMENTS_PER_PAGE" ] + ( $low < 0 ? $low : 0 ) );
        if ( $error !== "" )
            echo "<div class=\"main\"><font class=\"error\">" . $error . "</font></div>";
        else if ( count( $comments_list ) !== 0 ) {
            for ( $i = 0; $i < count( $comments_list ); ++$i ) {
                ?>
                <div class="<? echo $i % 2 == 0 ? "main" : "mainmid"; ?>" id="comment_<? echo $comments_list[ $i ][ "id" ] ?>">
                    <a href="?a=profile&amp;p=<? echo $comments_list[ $i ][ "author" ]; ?>"><? echo $comments_list[ $i ][ "author_name" ]; ?></a>
                    <font class="dark">wrote at</font> <? echo time_str( $comments_list[ $i ][ "date" ] );
                    if ( $logged[ "id" ] == $comments_list[ $i ][ "author" ] )
                        echo " (<a href=\"?a=edit_comment&amp;p=" . $comments_list[ $i ][ "id" ] . "#edit_comment\">edit</a>)";
                    ?><font class="dark">:</font>
                    <br><br>
                    <font class="comments"><? echo nl2br( htmlspecialchars( $comments_list[ $i ][ "comments" ] ) ); ?></font>
                </div>
                <?
            }
        }
        if ( $pages >= 2 )
            pages( $comments_count . " <font class=\"dark\">comments:</font> ", "?a=replay&amp;p=" . $replay[ "id" ], $pages, true );
    }
}

function do_download( $db, &$error, $logged, &$param )
{
    $replay = get_replay( $db, $error, intval( $param ) );
    if ( $replay[ "id" ] === 0 )
         return "replay";
         
    $s = $db->prepare( "UPDATE replays SET downloads = ? WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return "replay";
    }
    $d = $replay[ "downloads" ] + 1;
    $s->bind_param( "ii", $d, $replay[ "id" ] );
    $s->execute();

    header( "Location: replays/" . $replay[ "seed" ] . "/" . $replay[ "replay" ] . ".wrp" );
    return "replay";
}

function do_edit_replay( $db, &$error, $logged, $param )
{
    if ( !isset( $_POST[ "comments" ] ) )
        return "replay";

    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "edit_replay";
    }
    
    $replay = get_replay( $db, $error, $param );
    if ( $replay[ "id" ] === 0 )
        return "edit_replay";
    if ( $logged[ "id" ] !== $replay[ "uploader" ] ) {
        $error = "You don't have permission to do that.";
        unset( $GLOBALS[ "_POST" ][ "comments" ] );
        return "replay";
    }

    $comment = "";
    $s = $db->prepare( "UPDATE replays SET comments = ? WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return "edit_replay";
    }
    $comments = trim( $_POST[ "comments" ] );
    $s->bind_param( "si", $comments, $param );
    unset( $GLOBALS[ "_POST" ][ "comments" ] );
    $s->execute();
    header( "Location: ?a=replay&p=" . $param );
    return "replay";
}

function do_add_comment( $db, &$error, $logged, $param )
{
    if ( !isset( $_POST[ "comments" ] ) )
        return "replay";
    if ( strlen( $_POST[ "comments" ] ) < 2 ) {
        $error = "Please write at least 2 characters.";
        return "replay";
    }
        
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "replay";
    }

    if ( $logged[ "id" ] === 0 ) {
        $error = "You don't have permission to do that.";
        unset( $GLOBALS[ "_POST" ][ "comments" ] );
        return "replay";
    }
    $replay = get_replay( $db, $error, $param );
    if ( $replay[ "id" ] === 0 )
        return "replay";
        
    $s = $db->prepare( "INSERT INTO comments (author, replay, date, comments) VALUES (?, ?, NOW(), ?)" );
    if ( !$s ) {
        $error = $db->error;
        return "replay";
    }
    $comments = trim( $_POST[ "comments" ] );
    $s->bind_param( "iis", $logged[ "id" ], $replay[ "id" ], $comments );
    $s->execute();
    unset( $GLOBALS[ "_POST" ][ "comments" ] );
    $s = $db->prepare( "SELECT LAST_INSERT_ID();" );
    if ( $s ) {
        $s->execute();
        $s->bind_result( $id );
        if ( $s->fetch() )
            header( "Location: ?a=replay&p=" . $replay[ "id" ] . "#comment_" . $id );
        while ( $s->fetch() );
    }
    return "replay";
}
        
function do_edit_comment( $db, &$error, $logged, &$param )
{
    if ( !isset( $_POST[ "comments" ] ) )
        return "edit_comment";
    if ( strlen( $_POST[ "comments" ] ) < 2 ) {
        $error = "Please write at least 2 characters.";
        return "edit_comment";
    }
        
    if ( !$db ) {
        $error = "Could not connect to the database. Please try again.";
        return "edit_comment";
    }

    $comment = get_comment( $db, $error, $param );
    if ( $comment[ "id" ] === 0 )
        return "edit_comment";
    $replay = get_replay( $db, $error, $comment[ "replay" ] );
    if ( $replay[ "id" ] === 0 )
        return "edit_comment";
    if ( $logged[ "id" ] !== $comment[ "author" ] ) {
        $error = "You don't have permission to do that.";
        unset( $GLOBALS[ "_POST" ][ "comments" ] );
        $param = $replay[ "id" ];
        return "replay";
    }
        
    $s = $db->prepare( "UPDATE comments set comments = ? WHERE id = ? LIMIT 1" );
    if ( !$s ) {
        $error = $db->error;
        return "edit_comment";
    }
    $comments = trim( $_POST[ "comments" ] );
    $s->bind_param( "si", $comments, $comment[ "id" ] );
    $s->execute();
    unset( $GLOBALS[ "_POST" ][ "comments" ] );
    $param = $replay[ "id" ];
    $rank = get_comment_ranking( $db, $error, $comment );
    $GLOBALS[ "PAGE" ] = intval( ( $rank - 1 ) / $GLOBALS[ "COMMENTS_PER_PAGE" ] );
    header( "Location: ?a=replay&p=" . $param . "&q=" . $GLOBALS[ "PAGE" ] . "#comment_" . $comment[ "id" ] );
    return "replay";
}

function scoreboard( $db, $error, $logged, $param, $order )
{
    $mode = intval( intval( $param ) / 5 );
    $players = intval( $param ) % 5;
    if ( $mode === 1 )
        $mode_str = "BOSS";
    else if ( $mode === 2 )
        $mode_str = "HARD";
    else if ( $mode === 3 )
        $mode_str = "FAST";
    else if ( $mode === 4 )
        $mode_str = "WHAT";
    else if ( $mode === 5 )
        $mode_str = "ALL";
    else
        $mode_str = "GAME";
        
    $replays_count = count_replays_by_mode( $db, $error, $mode_str, ( $players + 1 ) % 5 );
    $pages = 1 + intval( ( $replays_count - 1 ) / $GLOBALS[ "REPLAYS_PER_PAGE" ] );
    $GLOBALS[ "PAGE" ] = max( 0, min( $pages - 1, $GLOBALS[ "PAGE" ] ) );
    $low = $GLOBALS[ "PAGE" ] * $GLOBALS[ "REPLAYS_PER_PAGE" ];
    
    $bu = is_mode_unlocked( $db, $error, "BOSS" );
    $hu = is_mode_unlocked( $db, $error, "HARD" );
    $fu = is_mode_unlocked( $db, $error, "FAST" );
    $wu = is_mode_unlocked( $db, $error, "WHAT" );
    $au = $bu || $hu || $fu || $wu;
            
    $replays = get_replays_by_mode( $db, $error, $mode_str, ( $players + 1 ) % 5, $order, $low, $GLOBALS[ "REPLAYS_PER_PAGE" ] );
    ?>
    <div class="head">
        Hi-scores
    </div>
    <div class="first">
        <? if ( $logged[ "id" ] !== 0 ) { ?><div class="score"><a href="?a=upload">Upload a replay!</a></div><? } ?>
        <? if ( $error !== "" ) echo "<font class=\"error\">" . $error . "</font><br><br>"; ?>
        <? if ( $mode === 0 ) echo mode_str( "GAME", true, true, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . (  0 + $players ) . "&amp;o=" . $order . "\">" . mode_str( "GAME", true, false, true ) . "</a>"; ?>
        <? if ( $bu ) { if ( $mode === 1 ) echo mode_str( "BOSS", true, true, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . (  5 + $players ) . "&amp;o=" . $order . "\">" . mode_str( "BOSS", true, false, true ) . "</a>"; } ?>
        <? if ( $hu ) { if ( $mode === 2 ) echo mode_str( "HARD", true, true, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 10 + $players ) . "&amp;o=" . $order . "\">" . mode_str( "HARD", true, false, true ) . "</a>"; } ?>
        <? if ( $fu ) { if ( $mode === 3 ) echo mode_str( "FAST", true, true, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 15 + $players ) . "&amp;o=" . $order . "\">" . mode_str( "FAST", true, false, true ) . "</a>"; } ?>
        <? if ( $wu ) { if ( $mode === 4 ) echo mode_str( "WHAT", true, true, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 20 + $players ) . "&amp;o=" . $order . "\">" . mode_str( "WHAT", true, false, true ) . "</a>"; } ?>
        <? if ( $au ) { if ( $mode === 5 ) echo "ALL"; else echo "<a href=\"?a=scoreboard&amp;p=" . ( 25 + $players ) . "&amp;o=" . $order . "\">ALL</a>"; } ?>
        <br>
        <? if ( $players === 0 ) echo players_str( 1, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 5 * $mode + 0 ) . "&amp;o=" . $order . "\">" . players_str( 1, true, false ) . "</a>"; ?>
        <? if ( $players === 1 ) echo players_str( 2, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 5 * $mode + 1 ) . "&amp;o=" . $order . "\">" . players_str( 2, true, false ) . "</a>"; ?>
        <? if ( $players === 2 ) echo players_str( 3, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 5 * $mode + 2 ) . "&amp;o=" . $order . "\">" . players_str( 3, true, false ) . "</a>"; ?>
        <? if ( $players === 3 ) echo players_str( 4, true ); else echo "<a href=\"?a=scoreboard&amp;p=" . ( 5 * $mode + 3 ) . "&amp;o=" . $order . "\">" . players_str( 4, true, false ) . "</a>"; ?>
        <? if ( $players === 4 ) echo "ALL"; else echo "<a href=\"?a=scoreboard&amp;p=" . ( 5 * $mode + 4 ) . "&amp;o=" . $order . "\">ALL</a>"; ?>
        <br><br>
        <font class="dark">Order by:</font>
        <br>
        <? if ( $order === 0 ) echo "High"; else echo "<a href=\"?a=scoreboard&amp;p=" . $param . "&amp;o=0\">High</a>"; ?>
        <? if ( $order === 1 ) echo "Low"; else echo "<a href=\"?a=scoreboard&amp;p=" . $param . "&amp;o=1\">Low</a>"; ?>
        <? if ( $order === 2 ) echo "New"; else echo "<a href=\"?a=scoreboard&amp;p=" . $param . "&amp;o=2\">New</a>"; ?>
        <? if ( $order === 3 ) echo "Old"; else echo "<a href=\"?a=scoreboard&amp;p=" . $param . "&amp;o=3\">Old</a>"; ?>
    </div>
    <?
    if ( $pages >= 2 )
        pages( $replays_count . " <font class=\"dark\">replays:</font> ", "?a=scoreboard&amp;p=" . $param . "&amp;o=" . $order, $pages );
    ?>
    <div class="main">
        <? if ( count( $replays ) === 0 ) echo "<font class=\"dark\">(none)</font>"; else replay_list( $replays, true, $low ); ?>
    </div>
    <?
    if ( $pages >= 2 )
        pages( $replays_count . " <font class=\"dark\">replays:</font> ", "?a=scoreboard&amp;p=" . $param . "&amp;o=" . $order, $pages );
}

error_reporting( E_ALL | E_STRICT );
date_default_timezone_set( "UTC" );
session_start();
$DB = mysqli_connect( $DB_HOST, $DB_USERNAME, $DB_PASSWORD, $DB_DATABASE );

if ( isset( $_SESSION[ "id" ] ) && $_SESSION[ "id" ] > 0 ) {
    $LOGGED_USER = get_account( $DB, $ERROR, $_SESSION[ "id" ] );
    if ( $LOGGED_USER[ "id" ] !== 0 && ( $ACTION === "login" || $ACTION === "do_login" || $ACTION === "register" || $ACTION === "do_register" ) )
        $ACTION = "profile";
    if ( $LOGGED_USER[ "id" ] !== 0 && ( $ACTION === "profile" && $PARAM === "" ) )
        $PARAM = strval( $LOGGED_USER[ "id" ] );
}
if ( $LOGGED_USER[ "id" ] === 0 && ( $ACTION === "edit_profile" || $ACTION === "do_edit_profile" || $ACTION === "do_logout" || $ACTION === "do_upload" ) )
    $ACTION = "home";
if ( $LOGGED_USER[ "id" ] === 0 && ( $ACTION === "edit_replay" || $ACTION === "do_edit_replay" || $ACTION === "do_add_comment" || $ACTION === "edit_comment" || $ACTION === "do_edit_comment" ) )
    $ACTION = "replay";
if ( $ACTION === "scoreboard" && $PARAM === "" )
    $PARAM = "0";

if ( $FILE > 0 )
    do_static_file( $DB, $ERROR, $LOGGED_USER, $FILE );

else if ( $ACTION === "do_register" )
    $ACTION = do_register( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_login" )
    $ACTION = do_login( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_logout" )
    $ACTION = do_logout( $LOGGED_USER );
else if ( $ACTION === "do_edit_profile" )
    $ACTION = do_edit_profile( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_upload" )
    $ACTION = do_upload( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_download" )
    $ACTION = do_download( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_edit_replay" )
    $ACTION = do_edit_replay( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_add_comment" )
    $ACTION = do_add_comment( $DB, $ERROR, $LOGGED_USER, $PARAM );
else if ( $ACTION === "do_edit_comment" )
    $ACTION = do_edit_comment( $DB, $ERROR, $LOGGED_USER, $PARAM );

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
    <head>

        <title>WiiSPACE: arcade shmup</title>
        <meta http-equiv="content-type" content="text/html; charset=UTF-8">
        <meta name="description" content="WiiSPACE: arcade shmup">
        <meta name="keywords" content="seiken,wiispace,shmup,wii,space,shooter,video,game,arcade">
        <link rel="stylesheet" href="style.css" type="text/css">
	   <!--[if lt IE 7]>
	   <style media="screen" type="text/css">
	   div.contain {
            height:100%;
	   }
	   </style>
	   <![endif]-->
        <link rel="image_src" href="logo.png">
        <link rel="shortcut icon" href="favicon.ico">
        <link rel="icon" href="favicon.ico">
        <script type="text/javascript">
        var RecaptchaOptions = {
            theme: 'custom',
            custom_theme_widget: 'recaptcha_widget'
        };
        </script>
    
    </head>

    <body>
        <? background(); ?>
        <div class="contain">
            <div class="outer">
                <?
                head( $LOGGED_USER );
                if ( $ACTION === "register" )
                    register_form( $ERROR );
                else if ( $ACTION === "login" )
                    login_form( $ERROR );
                else if ( $ACTION === "profile" )
                    profile( $DB, $ERROR, $LOGGED_USER, get_account( $DB, $ERROR, intval( $PARAM ) ), false );
                else if ( $ACTION === "edit_profile" )
                    profile( $DB, $ERROR, $LOGGED_USER, $LOGGED_USER, true );
                else if ( $ACTION === "profile_comments" )
                    profile_comments( $DB, $ERROR, $LOGGED_USER, get_account( $DB, $ERROR, intval( $PARAM ) ) );
                else if ( $ACTION === "player_list" )
                    player_list( $DB, $ERROR, $ORDER );
                else if ( $ACTION === "upload" )
                    upload_form( $ERROR );
                else if ( $ACTION === "replay" )
                    replay( $DB, $ERROR, $LOGGED_USER, get_replay( $DB, $ERROR, $PARAM === "r" ? $PARAM : intval( $PARAM ) ), false );
                else if ( $ACTION === "edit_replay" )
                    replay( $DB, $ERROR, $LOGGED_USER, get_replay( $DB, $ERROR, intval( $PARAM ) ), true );
                else if ( $ACTION === "edit_comment" ) {
                    $comment = get_comment( $DB, $ERROR, $PARAM );
                    $replay = $comment[ "id" ] !== 0 ? get_replay( $DB, $ERROR, $comment[ "replay" ] ) : array( "id" => 0 );
                    replay( $DB, $ERROR, $LOGGED_USER, $replay, false, $comment );
                }
                else if ( $ACTION === "scoreboard" )
                    scoreboard( $DB, $ERROR, $LOGGED_USER, $PARAM, $ORDER );
                else
                    home_page( $ERROR, $LOGGED_USER );
                ?>
            </div>
            <? foot(); ?>
        </div>
    </body>
</html>
