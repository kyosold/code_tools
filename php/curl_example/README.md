curl_post
=======

Intro
-----

使用curl模拟post提交数据，里面包含读写cookie，处理https等。


教程
------------

#### 命令行执行

```bash
$ php ./curl_post.php
```

#### 使用方式
```bash
$url:               提交的URL
$method:            GET 或 POST
$post_data:         post提交的数据
save_cookie_file:   保存取到的cookie的文件
send_cookie_file:   需要发送到服务器的cookie文件

// 比如：
// 登录使用user:abc password:123 登录url: https://passport.xxx.com/login.php
// 需要保存cookie到文件: ./cookie.txt
$url = 'https://passport.xxx.com/login.php';
$method = 'POST';
$post_data = array(
        'user'      => 'abc',
        'password'  => '123'
    );
$save_cookie_fs = './cookie.txt';
$send_cookie_fs = '';

$res = submit_curl_action($url, $method, $post_data, $save_cookie_fs, $send_cookie_fs);
var_dump($res);
```

