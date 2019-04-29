
---
title: spark quick start
date: 2019-04-28 19:52:36
tags: [spark]
---

注意， Spark2.0 之前的版本，Spark的主要编程接口是RDD(Resilient Distributed Dataset)。2.0以后的版本，主要编程接口替换为Dataset。当然了RDD接口依然支持，可以从[RDD programming guide](https://spark.apache.org/docs/latest/rdd-programming-guide.html)。但是，我们强烈建议你切换到Dataset，比RDD有更好的性能。 关于Dataset可以从[SQL programming guide](https://spark.apache.org/docs/latest/sql-programming-guide.html)获得更多细节。


### 使用SparkShell进行交互分析

#### 基础内容
启动 ./bin/spark-shell


基本执行：

    val textFile = spark.read.textFile("README.md")
    textFile.count() // Number of items in this Dataset
    textFile.first() // first item in this Dataset
    
将一个Dataset转换为新的一个。 调用`filter`返回新的Dataset，是原始文件的一个子集

    val linesWithSpark = textFile.filter(t => t.contains("Spark"))
    
    
#### 这儿有个疑问，如何打印出dataset中内容？？


#### 更多的Dataset的操作

Dataset的actions和transformation 可以用于更多的复杂运算。


    textFile.map(line => line.split(" ").size).reduce((a,b) => if (a > b) a else b)
    res4: Long = 15
    
map和reduce的参数都是scala的匿名函数，还可以使用scala/java 库。例如 可以使用`Math.max()`函数。

    import java.lang.Math
    textFile.map(line => line.split(" ").size).reduce((a, b) => Math.max(a,b))

一个通用的数据处理流程是MapReduce。Spark可以很容易的实现MapReduce流。 

    val wordCounts = textFile.map(line => line.split(" ")).groupByKey(identity).count()
    
    
#### Caching

spark支持将dataset写入cluster-wide in-memory cache。
当数据重复获取时，这还是很有用的。 


#### Self-Contained Application

使用SparkAPI创建一个self-contained应用。

    
    import org.apache.spark.sql.SparkSession

    object SimpleApp {
      def main(args: Array[String]) {
        val logFile = "README.md" // Should be some file on your system
        val spark = SparkSession.builder.appName("Simple Application").getOrCreate()
        val logData = spark.read.textFile(logFile).cache()
        val numAs = logData.filter(line => line.contains("a")).count()
        val numBs = logData.filter(line => line.contains("b")).count()
        println(s"Lines with a: $numAs, Lines with b: $numBs")
        spark.stop()
      }
    }
    
    
其中`SparkSession.builder`构造一个[[SparkSession]]，使用设置application name，最后调用`getOrCreate`获取[[SparkSession]]实例。


也可以使用maven来进行包管理。 

目录结构：

    ./pom.xml
    ./src
    ./src/main
    ./src/main/scala
    ./src/main/scala/didi
    ./src/main/scala/didi/map
    ./src/main/scala/didi/map/pointsys
    ./src/main/scala/didi/map/pointsys/App.scala
    ./src/test
    ./src/test/scala
    ./src/test/scala/didi
    ./src/test/scala/didi/map
    ./src/test/scala/didi/map/pointsys

App.scala内容：
    
    package didi.map.pointsys

    import org.apache.spark.sql.SparkSession
    object MyFunctions {
      def func1(s: String): String = {
        s.concat("yankai")
      }
    }
    // spark-submit --class="didi.map.pointsys.SimpleApp" parking-user-profile-1.0-SNAPSHOT.jar
    object SimpleApp {
      def main(args: Array[String]) {
        val logFile = "README.md" // Should be some file on your system
        val ss = SparkSession.builder().appName("Simple Application").enableHiveSupport().getOrCreate()
        val logData = ss.read.textFile(logFile)
        //val pairs = logData.map(s => (s, 1))
    
        val numAs = logData.filter(line => line.contains("a")).count()
        val numBs = logData.filter(line => line.contains("b")).count()
        println(s"Lines with a: $numAs, Lines with b: $numBs")
        ss.stop()
      }
    }





pom.xml

    <project xmlns="http://maven.apache.org/POM/4.0.0" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 http://maven.apache.org/maven-v4_0_0.xsd">
      <modelVersion>4.0.0</modelVersion>
      <groupId>didi.map.pointsys</groupId>
      <artifactId>parking-user-profile</artifactId>
      <version>1.0-SNAPSHOT</version>
      <inceptionYear>2008</inceptionYear>
      <properties>
        <encoding>UTF-8</encoding>
        <scala.binary.version>2.11</scala.binary.version>
        <scala.major.version>2.11</scala.major.version>
        <deploy.scala.version>2.11</deploy.scala.version>
        <scala.version>2.11.8</scala.version>
        <scala.compat.version>2.11</scala.compat.version>
        <spark.version>2.2.0</spark.version>
      </properties>
    
      <dependencies>
        <dependency>
          <groupId>junit</groupId>
          <artifactId>junit</artifactId>
          <version>4.12</version>
          <scope>test</scope>
        </dependency>
    
        <dependency>
          <groupId>org.apache.spark</groupId>
          <artifactId>spark-core_2.11</artifactId>
          <version>2.3.0</version>
          <scope>provided</scope>
        </dependency>
        <dependency>
          <groupId>org.apache.spark</groupId>
          <artifactId>spark-sql_2.11</artifactId>
          <version>2.3.0</version>
          <scope>provided</scope>
        </dependency>
        <dependency>
          <groupId>org.apache.hadoop</groupId>
          <artifactId>hadoop-client</artifactId>
          <version>2.7.2</version>
          <scope>provided</scope>
        </dependency>
    
      </dependencies>
      <build>
        <sourceDirectory>src/main/scala</sourceDirectory>
        <testSourceDirectory>src/test/scala</testSourceDirectory>
        <plugins>
          <!-- bind the maven-assembly-plugin to the package phase this will create
              a jar file without the storm dependencies suitable for deployment to a cluster. -->
    
          <plugin>
            <groupId>net.alchim31.maven</groupId>
            <artifactId>scala-maven-plugin</artifactId>
            <version>3.2.0</version>
            <executions>
              <execution>
                <goals>
                  <goal>compile</goal>
                  <goal>testCompile</goal>
                </goals>
              </execution>
            </executions>
            <configuration>
              <scalaVersion>${scala.version}</scalaVersion>
            </configuration>
          </plugin>
    
          <plugin>
            <groupId>org.apache.maven.plugins</groupId>
            <artifactId>maven-assembly-plugin</artifactId>
            <version>2.2-beta-5</version>
            <configuration>
              <descriptorRefs>
                <descriptorRef>jar-with-dependencies</descriptorRef>
              </descriptorRefs>
            </configuration>
            <executions>
              <execution>
                <phase>package</phase>
                <goals>
                  <goal>single</goal>
                </goals>
              </execution>
            </executions>
          </plugin>
    
          <plugin>
            <groupId>org.apache.maven.plugins</groupId>
            <artifactId>maven-compiler-plugin</artifactId>
            <version>3.5.1</version>
            <configuration>
              <source>1.8</source>
              <target>1.8</target>
            </configuration>
          </plugin>
          <!-- disable surefire -->
          <plugin>
            <groupId>org.apache.maven.plugins</groupId>
            <artifactId>maven-surefire-plugin</artifactId>
            <version>2.7</version>
            <configuration>
              <skipTests>true</skipTests>
            </configuration>
          </plugin>
          <!-- enable scalatest -->
          <!-- <plugin>
            <groupId>org.scalatest</groupId>
            <artifactId>scalatest-maven-plugin</artifactId>
            <version>1.0</version>
            <configuration>
              <reportsDirectory>${project.build.directory}/surefire-reports</reportsDirectory>
              <junitxml>.</junitxml>
              <filereports>WDF TestSuite.txt</filereports>
            </configuration>
            <executions>
              <execution>
                <id>test</id>
                <goals>
                  <goal>test</goal>
                </goals>
              </execution>
            </executions>
          </plugin>
          -->
        </plugins>
    
        <resources>
          <resource>
            <directory>src/main/resources</directory>
          </resource>
        </resources>
    
      </build>
    </project>

在target目录下会生成jar文件，使用spark-submit进行提交。

    spark-submit --class="didi.map.pointsys.SimpleApp" parking-user-profile-1.0-SNAPSHOT.jar
