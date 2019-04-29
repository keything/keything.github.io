
Starting Point: SparkSession

SparkSession in Spark 2.0 provides builtin support for Hive features including the ability to write queries using HiveQL, access to Hive UDFs, and the ability to read data from Hive tables. To use these features, you do not need to have an existing Hive setup.


### 1. 创建DataFrames

With a SparkSession, applications can create DataFrames from an existing RDD, from a Hive table, or from Spark data sources.

使用SparkSession，应用可以从已经存在的RDD、Hive table、Spark Data sources中创建DataFrames。

下面的例子是基于json文件的内容创建一个DataFrame

    val df = spark.read.json("examples/src/main/resources/people.json")

    // Displays the content of the DataFrame to stdout
    df.show()
    // +----+-------+
    // | age|   name|
    // +----+-------+
    // |null|Michael|
    // |  30|   Andy|
    // |  19| Justin|
    // +----+-------+


#### 1.1  Untyped Dataset Operations (aka DataFrame Operations)

    // This import is needed to use the $-notation
    import spark.implicits._
    // Print the schema in a tree format
    df.printSchema()
    // root
    // |-- age: long (nullable = true)
    // |-- name: string (nullable = true)
    
    // Select only the "name" column
    df.select("name").show()
    // +-------+
    // |   name|
    // +-------+
    // |Michael|
    // |   Andy|
    // | Justin|
    // +-------+
    
    // Select everybody, but increment the age by 1
    df.select($"name", $"age" + 1).show()
    // +-------+---------+
    // |   name|(age + 1)|
    // +-------+---------+
    // |Michael|     null|
    // |   Andy|       31|
    // | Justin|       20|
    // +-------+---------+
    
    // Select people older than 21
    df.filter($"age" > 21).show()
    // +---+----+
    // |age|name|
    // +---+----+
    // | 30|Andy|
    // +---+----+
    
    // Count people by age
    df.groupBy("age").count().show()
    // +----+-----+
    // | age|count|
    // +----+-----+
    // |  19|    1|
    // |null|    1|
    // |  30|    1|
    // +----+-----+
    
### 1.2 Running SQL Queries Programmatically

The sql function on a SparkSession enables applications to run SQL queries programmatically and returns the result as a DataFrame.
    
    // Register the DataFrame as a SQL temporary view
    df.createOrReplaceTempView("people")
    
    val sqlDF = spark.sql("SELECT * FROM people")
    sqlDF.show()
    // +----+-------+
    // | age|   name|
    // +----+-------+
    // |null|Michael|
    // |  30|   Andy|
    // |  19| Justin|
    // +----+-------+
    
    
    
    
### 1.3. Global Temporary View

Temporary views in Spark SQL are session-scoped and will disappear if the session that creates it terminates. If you want to have a temporary view that is shared among all sessions and keep alive until the Spark application terminates, you can create a global temporary view. Global temporary view is tied to a system preserved database global_temp, and we must use the qualified name to refer it, e.g. SELECT * FROM global_temp.view1.



### 2. Creating Datasets
Datasets are similar to RDDs, however, instead of using Java serialization or Kryo they use a specialized Encoder to serialize the objects for processing or transmitting over the network. While both encoders and standard serialization are responsible for turning an object into bytes, encoders are code generated dynamically and use a format that allows Spark to perform many operations like filtering, sorting and hashing without deserializing the bytes back into an object.



### 2.1 与RDDs的交互

有两种方式可以将已经存在RDDs转换为Datasets。

+ 使用reflection推断RDD的schema。基于reflection的方法代码更简洁
+ 通过一个可编程的接口，允许用户自己构造一个schema，并将其应用于已经存在的RDD。当columns及其类型在运行时还不确定时，可以用于构造Dataset。 

#### 2.1.1 使用反射推断schema

Spark Sql的scala接口自动将一个包含`case classes`的RDD 转换为DataFrame。`case class`定义了table的schema。使用反射读取`case class`中参数名称，这些名称会成为列的名称。 `case class`可以嵌套 或 包含复杂类型，比如`Seqs`或 `Arrays`。
RDD可以转换为DataFrame，并在table中注册。Tables可以用subsequent SQL 使用。 
    
    // For implicit conversions from RDDs to DataFrames
    import spark.implicits._
    case class Person(name: String, age: Long)

    // Create an RDD of Person objects from a text file, convert it to a Dataframe
    val peopleDF = spark.sparkContext
      .textFile("examples/src/main/resources/people.txt")
      .map(_.split(","))
      .map(attributes => Person(attributes(0), attributes(1).trim.toInt))
      .toDF()
    // Register the DataFrame as a temporary view
    peopleDF.createOrReplaceTempView("people")
    
    // SQL statements can be run by using the sql methods provided by Spark
    val teenagersDF = spark.sql("SELECT name, age FROM people WHERE age BETWEEN 13 AND 19")
    
    // The columns of a row in the result can be accessed by field index
    teenagersDF.map(teenager => "Name: " + teenager(0)).show()
    // +------------+
    // |       value|
    // +------------+
    // |Name: Justin|
    // +------------+
    
    // or by field name
    teenagersDF.map(teenager => "Name: " + teenager.getAs[String]("name")).show()
    // +------------+
    // |       value|
    // +------------+
    // |Name: Justin|
    // +------------+
    
    // No pre-defined encoders for Dataset[Map[K,V]], define explicitly
    implicit val mapEncoder = org.apache.spark.sql.Encoders.kryo[Map[String, Any]]
    // Primitive types and case classes can be also defined as
    // implicit val stringIntMapEncoder: Encoder[Map[String, Any]] = ExpressionEncoder()
    
    // row.getValuesMap[T] retrieves multiple columns at once into a Map[String, T]
    teenagersDF.map(teenager => teenager.getValuesMap[Any](List("name", "age"))).collect()
    // Array(Map("name" -> "Justin", "age" -> 19))


#### 2.1.2 可编程指定schema

### 3. Aggregations


#### 3.1 Untyped User-Defined Aggregate Functions

#### 3.2 Type-Safe User-Defined Aggregate Functions
