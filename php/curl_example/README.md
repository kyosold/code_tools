curl_post
=======

Intro
-----

使用curl模拟post提交数据，里面包含读写cookie，处理https等。


教程
------------

#### 命令行执行

```bash
$ php ./curl_post.php <save_cookie> <send_cookie>
```

#### 说明
需要修改:
$url:       提交的URL
$post_data: post提交的数据

<save_cookie>: 保存取到的cookie的文件
<send_cookie>: 需要发送到服务器的cookie文件

