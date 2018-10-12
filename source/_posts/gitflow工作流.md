---
title: gitflow工作流
date: 2016-03-15 08:23:25
tags: git
author: yankai
---

## 概述
### 一. 现有开发方式

在原有的开发中，我们的工作方式有两种，一是master上直接修改，二是开一个分支feature，开发测试直接合并master。

存在的问题：

1. 代码不能进行review
2. master分支不能保持可用状态

### 二. 引入pull request

我们引入pull request的目的很简单，就是进行code review。code review对比的是两个分支。

接下来，我们将会介绍两种工作方式，我们将采用gitflow这种工作流，为了让大家有个更加清晰的了解，我们先介绍单纯采用分支的工作方式，然后再介绍采用gitflow工作流的方式。
### 三. 最简单的采用分支的工作方式
最简单的方式，我们所有的开发（包括简单的一次修改和复杂的新功能开发）都是走分支，分支开发完毕以后跟master进行比较，从而进行code review。流程简单如下


![simple](/images/gitflow/simple_br.png)

	
	1. 从master分支checkout出feature/fa分支
	2. 在feature/fa分支进行开发，每个节点代表一次修改和一次commit
	3. 将本地的feature/fa 推到远程仓库，从而其他小伙伴能够拉取代码
	4. 分支feature/fa开发完毕以后，开始进行code view，我们需要到xxxx..net上创建一个merge request。
		4.1 如果在review中发现代码修改，那么在feature/fa分支进行修改，然后再将feature/fa分支push到远程仓库即可
	5. 分支开发完毕，现在切到master分支，在master分支上拉取远程仓库origin的master分支
	6. 将feature/fa分支合并到mater分支上。
	7. 将master分支推到远程仓库origin的master从而完成开发。

### 四. 引入gitflow工作流

（一） gitflow的简要介绍

在gitflow中有两个永久存在的分支develop和master分支，有很多临时的功能分支feature,release,hotfix。详细介绍可以看 http://nvie.com/posts/a-successful-git-branching-model/

（二） 我们如何使用gitflow

结合我们项目特点，目前我们使用阉割版的gitflow，不使用它的hotfix和release分支，仅仅使用master,develop,feature三个分支。流程如下：

![gitflow](/images/gitflow/gitflow.png)

	1. 开辟一个feature分支，我们使用gitflow命令，好处就是他默认从develop分支checkout出一个分支
	2. 正常的分支开发，每个节点代表依次修改和commit
	3. 将本地的feature/fa 推到远程仓库，从而其他小伙伴能够拉取代码
	4. 分支feature/fa开发完毕以后，开始进行code view，我们需要到xxxxx..net上创建一个merge request。
		4.1 如果在review中发现代码修改，那么在feature/fa分支进行修改，然后再将feature/fa分支push到远程仓库即可
	5. 将feature/fa分支合并到develop分支，并删除feature/fa分支，听着很复杂，实际上只需要执行一条命令 git flow feature finish fa即可
	6. 将develop分支推送到远程仓库，之所以这样是为了让小伙伴同步代码
	7. 分支开发完毕，现在切到master分支，在master分支上拉取远程仓库origin的master分支
	8. 将develop分支合并到mater分支上。
	9. 将master分支推到远程仓库origin的master从而完成开发。


### 四. 受限分支master

1. 目前存在问题

	到目前为止，我们引入了pull request进行codereview，引入工作流gitflow规范大家的开发流程。但是！但是！master分支依然允许所有的人进行操作，那么可能有人不遵循规则，而对master进行修改和上线。

2. 解决方法
	
	通过gitlab，我们对master分支进行保护，即受限分支。只有具有权限的小伙伴才能对远程仓库的master分支进行操作。当然了，你操作你本地的master分支木有问题，但你想向远程的master分支推送，不好意思，你被禁止了。
	
3. 引入受限分支以后，开发流程的变化

	在上面描述的流程中，gitflow的7，8，9三步交给具有操作master分支权限的小伙伴来执行。
	
### 五 参考文章
http://nvie.com/posts/a-successful-git-branching-model/ 
