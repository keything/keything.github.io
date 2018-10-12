---
title: php发送邮件
date: 2016-05-25 16:31:31
tags: php
---

## 0. 概述
php manual中关于mail的介绍很简单，
    
    @param to 电子邮件收件人或收件人列表
    @param subjct 电子邮件的主题 也就是收件人或收件人列表
    @param message 所要发送的消息 
    @return true if the mail was successfully accepted fro delivery, FALSE otherwise
    bool mail ( string $to , string $subject , string $message [, string $additional_headers [, string $additional_parameters ]] )

## 0. 我的代码
根据邮件协议，使用基本的php代码进行编写，直接包含这个类即可使用
## 1. 发送html
1. 发送html就是将正文的message以文本格式发送
## 2. 发送附件
1. 附件需要以multipart/form-data的格式进行组装，这儿与web上传form表单的格式一样。例子如下

        --==Mime_Multipart_Boundary_x69c8f8864502559be8e17a0cb379ff0cx
        Content-Transfer-Encoding: base64

        54eV5YevQm9keQoKCgo=

        --==Mime_Multipart_Boundary_x69c8f8864502559be8e17a0cb379ff0cx
        X-Attachment-Id: 15259
        Content-Transfer-Encoding: base64
        Content-Type: application/octet-stream; name="keything.txt"
        Content-Disposition: attachment; filename="keything.txt"

        54eV5Yev

        --==Mime_Multipart_Boundary_x69c8f8864502559be8e17a0cb379ff0cx--
2. 我们所发出的数据就如上面文本所示，因此要将其组装


3. class KTMail

        <?php
        class KTMail
        {
            const UTF8_PREFIX  = '=?UTF-8?B?';
            const UTF8_POSTFIX = '?=';
            const RN = "\r\n";
            /**
             * @param from: who send this mail
             * @param to: who receive this mail
             * @param subject: the title of mail
             * @param body: the content of mail
             * @param attachment_fname: the file name of attachement
             * @param attachment_fdata: the file content of attachment
             * @return true if send succeed, FALSE otherwise
             * TODO if attachment_fname is chinese, it will be messay code.
             */
            public static function sendWithAttach($from, $to, $subject, $body, $attachment_fname, $attachment_fdata)
            {
                // a random string 
                $semi_rand = md5(time()); 
                $mime_boundary = '==Mime_Multipart_Boundary_x' . $semi_rand . 'x';
                $part_boundary = '==Part_Multipart_Boundary_x' . $semi_rand . 'x';

                // header 
                $headers = [];
                $headers[] = 'MIME-Version: 1.0';
                $headers[] = 'Content-Type: multipart/mixed; boundary=' . $mime_boundary;
                $headers[] = 'From: ' . $from;
                $headers_raw = implode(self::RN, $headers);

                // Message Body
                $msg = [];
                $msg[] = '--' . $mime_boundary;
                $msg[] = 'Content-Transfer-Encoding: base64' . self::RN;
                $msg[] =  chunk_split(base64_encode($body));

                // Attachment
                $msg[] = '--' . $mime_boundary;
                $msg[] = 'X-Attachment-Id: ' . rand(1000, 99999);
                $msg[] = 'Content-Transfer-Encoding: base64';
                $msg[] = 'Content-Type: application/octet-stream;' . ' name="' . $attachment_fname . '"';
                $msg[] = 'Content-Disposition: attachment; filename="'. $attachment_fname . '"' . self::RN;
                $msg[] = chunk_split(base64_encode($attachment_fdata));
                $msg[] = '--' . $mime_boundary . '--';

                $msg_raw = implode(self::RN, $msg);
                error_log($msg_raw, 3, '/tmp/sendmail.log');

                $real_subject = self::UTF8_PREFIX . base64_encode($subject) . self::UTF8_POSTFIX;

                return mail($to, $real_subject, $msg_raw, $headers_raw);
            }
            public static function sendWithHtml($from, $to, $subject, $body)
            {
                // header 
                $headers = array();
                $headers[] = 'MIME-Version: 1.0';
                $headers[] = 'Content-type: text/html; charset=utf-8';
                $headers[] = 'From: ' . $from;
                $headers_raw = implode(self::RN, $headers);

                // Message Body
                $real_subject = self::UTF8_PREFIX . base64_encode($subject) . self::UTF8_POSTFIX;
                $msg_raw = $body;

                return mail($to, $real_subject, $msg_raw, $headers_raw);
            }
        }

4. 测试用例

        <?php
        include ('ktmail.php');
        $from = 'local@a.cn';
        $to = 'your-email';
        $subject = 'keything.net';
        $body = 'keything.net';
        $attachment_fname = 'keything.txt';
        $attachment_fdata = 'keything';
        $attach_res = KTMail::sendWithAttach($from, $to, $subject, $body, $attachment_fname, $attachment_fdata);
        echo 'attach_res = ' . var_export($attach_res, true)."\n";

        $html_body = '
            <html>
                <head> keything </head>
                <body> 
                    keything body 
                    <table border="1">
                        <tr>
                            <td>row 1, cell 1</td>
                            <td>row 1, cell 2</td>
                        </tr>
                        <tr>
                            <td>row 2, cell 1</td>
                            <td>row 2, cell 2</td>
                        </tr>
                    </table>
                </body>
                </head>
            </html>
            ';
        $html_res = KTMail::sendWithHtml($from, $to, $subject, $html_body);
        echo 'html_res = ' . var_export($html_res, true)."\n";
