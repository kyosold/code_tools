<?php

function submit_curl_action($url, $method, $post_data = array(),  $save_cookie_file = '', $send_cookie_file = '')
{
    // 初始化
    $ch = curl_init();
    
    // 设置抓取的url
    curl_setopt($ch, CURLOPT_URL, $url);
    
    // 设置提交方式
    if (strtoupper($method) == 'POST') {
        curl_setopt($ch, CURLOPT_POST, 1);
        
        // 设置POST 数据
        curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
    }
    
    // 设置SSL
    $url_info = parse_url($url);
    if (strtoupper($url_info['scheme']) == 'HTTPS') {
        curl_setopt($ch, CURLOPT_CAINFO, dirname(__FILE__)."/cacert.pem");
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, TRUE);
    }
    
    // 设置保存 cookie 到文件
    if ($save_cookie_file != '') {
        curl_setopt($ch, CURLOPT_COOKIESESSION, 1);
        curl_setopt($ch, CURLOPT_COOKIEJAR, $save_cookie_file);
    }
    
    // 设置发送的cookie 
    if ($send_cookie_file != '') {
        curl_setopt($ch, CURLOPT_COOKIEFILE, $send_cookie_file);
    }
    
    // 设置头文件的信息作为数据流输出
    curl_setopt($ch, CURLOPT_HEADER, 1);
    
    // 设置获取的信息以文件流的形式返回，而不是直接输出
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    
    // 设置连接超时
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 2);
    
    // 设置读取返回数据的超时
    curl_setopt($ch, CURLOPT_TIMEOUT, 2);
    
    // ----- Exec -----
    $data = curl_exec($ch);
    
    // ----- Parse -----
    $get_info = curl_getinfo($ch);
    
    // 关闭URL请求
    curl_close($ch);
    
    
    return array(
        'getinfo' => $get_info,
        'data' => $data
    );
}

?>
