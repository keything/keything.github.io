---
title: git基本操作
date:   2015-12-20 13:02:01 +0800
tags: git
---

## 0. 初始化

    git init --bare 创建一个仓库

    git clone user@host:/path/repo.git

## 1. Git 用户配置

	1. 用户和邮箱
	
			git config --global user.name "Example Surname"
			git config --global user.email "your.email@gmail.com"

	2. 设置提交的分支
		
		simple只是将当前激活态的提交
		matching则是全部提交
		建议使用simple
	
			git config --global push.default simple 

	3. 设置高亮

		
			git config --global color.ui true 
			git config --global color.status auto 
			git config --global color.branch auto

	4. 默认的编辑器

			git config --global core.editor vim
			git config --global merge.tool vimdiff
			git config --global alias.d 'difftool -y'


			git config --global alias.staged 'diff --cached'
			git config --global alias.st 'status'
    5. 设置git status时显示中文

        git config --global core.quotepath false


## 2. 设置忽略文件


		.gitignore

## 3. 基本操作

		add rm commit branch push pull log status

## 4. 恢复文件

	从stage阶段恢复一个文件

		git reset HEAD xxx_file_

	从modify恢复一个文件
	
		git checkout -- xxx_file


		git commit -m 'message'
		git commit --amend -m "new message"

## 5. 从仓库中删除一个文件

	
		git rm -r --cached xxxfile
		git rm xxfile 二者区别


## 6. 远程操作

		Git remote add xxx_name yyy_url
		git remote rm xxx_name
		Git push repo_name xxx_branch

## 7. 分支操作

		Git branch
		git checkout -b xx_branch
		git branch -a // 包含远程分支

## 8. 分支之间的区别


		Git diff master your_branch

## 9. 标签 tag

		Git tag
		git tag 1.7.1 // 创建轻量的tag
		git show 1.7.1

		git tag 1.6.1 -m "release 1.6.1"
		git checkout <tag_name>
		git push origin [tag_name]
		git push origin tag <tag_name>
		git tag -d <tag_name>
		git push origin :refs/tags/1.6.1


## 10. 储藏

		Git stash
		git stash pop
		git stash
		git stash list
		git stash apply stash@{0}
		git stash drop stash@{0}
		git stash clear


## 11. 通过git reset 移动HEAD指针

		git reset --mixed HEAD~2

		git reflog

		git reset --soft HEAD~2

		git reset --hard HEAD~2


## 12. 恢复丢失的提交

		git reflog: is a mechanism to record the movements of the HEAD pointer and the branches


## 13. 远程分支删除

		 remote and local tracking branches
		  git branch -r // list all remote branches

		  git branch -d -r origin/[remote_branch] // delete remote branch from origin

		  git branch origin :branch_name
		  git push origin --delete branch_name

## 14. 解决冲突

    git mergetool

		<<<<<<< HEAD Change in the first repository 
		======= Change in the second repository 
		>>>>>>> b29196692f5ebfd10d8a9ca1911c8b08127c85f8

## 15. git log

    git log --since=1.days --graph --pretty=format:"%cn %d-->%s"

## 16. git archive

    git archive <COMMIT_ID> > destFile.tar.gz
    将提交id为COMMIT_ID的版本提取出来，以tar.gz的格式将其保存到destFile.tar.gz中

    其中COMMIT_ID可以替换为分支名，就是将当前分支的最新版本提取出来保存到目标文件
