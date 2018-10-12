---
title: svn迁移git-完美保留日志
date: 2016-03-11 08:20:12
tags: git
---

迁移流程--完美保留日志

1. 生成用户列表

    ``svn log --xml | grep author | sort -u |   perl -pe 's/.*>(.*?)<.*/$1 = $1 <$1\@compony.suffix>/' > users.txt``

2. 从svn克隆到新的目录

    git svn clone https://your_svn_reposity/project_name/ --authors-file=users.txt --no-metadata -s git_project_name

3. 从svn仓库拉取文件到新的目录

    cd git_project_name
    git svn fetch

4. 校验文件是否一致：

    diff -rbB --brief svn_cloudfs git_cloudfs | grep -v .svn

5. 添加远程仓库

    git remote add origin git@xxxx.net:xxx.git

6. 将代码上传到git仓库

    git push
