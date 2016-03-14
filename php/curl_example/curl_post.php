<?php

// 登录的URL
$url = 'http://passport.xxxx.com/login.php';

// Request头数据
$request_headers = array(
    'Host: passport.xxx.com',
    'User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:44.0) Gecko/20100101 Firefox/44.0 FirePHP/0.7.4',
    'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    'Accept-Language: en-US,en;q=0.5',
    'Referer: http://passport.xxxx.com/login.php'
);

// Post的数据 
$post_data = array(
    'username' => 'kyosold',
    'password' => '123qweasd'
);

$save_cookie = $argv[1];
$send_cookie = $argv[2];


$use_ssl = 0;
$url_info = parse_url($url);
if ( strtoupper($url_info['scheme']) == 'HTTPS' ) {
    $use_ssl = 1;
}


// 初始化 curl
$ch = curl_init();

// 设置 POST 方式提交, 如果是GET则该条不设置
curl_setopt($ch, CURLOPT_POST, 1);

// 设置 POST 数据
curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);

// 设置 ssl 
if ($use_ssl == 1) {
    curl_setopt($ch, CURLOPT_CAINFO, dirname(__FILE__)."/cacert.pem");
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, TRUE);
}

// 设置保存 cookie 到文件
curl_setopt($ch, CURLOPT_COOKIESESSION, 1);
curl_setopt($ch, CURLOPT_COOKIEJAR, $save_cookie);

// 设置Header自定义头
curl_setopt($ch, CURLOPT_HTTPHEADER, $request_headers);


// 设置抓取的url
curl_setopt($ch, CURLOPT_URL, $url);
// 设置发送的cookie 
curl_setopt($ch, CURLOPT_COOKIEFILE, $send_cookie);
// 设置头文件的信息作为数据流输出
curl_setopt($ch, CURLOPT_HEADER, 1);
// 设置获取的信息以文件流的形式返回，而不是直接输出
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);


// 执行命令
$data = curl_exec($ch);

// 关闭URL请求
curl_close($ch);

print_r($data);


?>