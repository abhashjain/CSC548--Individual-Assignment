/*
* CSC548 - HW5 Question 1- Calculate TFIDF using the Spark on set of document
*  Author: ajain28 Abhash Jain
*
*/
import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaPairRDD;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.api.java.function.*;
import scala.Tuple2;

import java.util.*;

/*
 * Main class of the TFIDF Spark implementation.
 * Author: Tyler Stocksdale
 * Date:   10/31/2017
 */

public class TFIDF {

	static boolean DEBUG = true;

    public static void main(String[] args) throws Exception {
        // Check for correct usage
        if (args.length != 1) {
            System.err.println("Usage: TFIDF <input dir>");
            System.exit(1);
        }
		
		// Create a Java Spark Context
		SparkConf conf = new SparkConf().setAppName("TFIDF");
		JavaSparkContext sc = new JavaSparkContext(conf);

		// Load our input data
		// Output is: ( filePath , fileContents ) for each file in inputPath
		String inputPath = args[0];
		JavaPairRDD<String,String> filesRDD = sc.wholeTextFiles(inputPath);
		
		// Get/set the number of documents (to be used in the IDF job)
		long numDocs = filesRDD.count();
		
		//Print filesRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = filesRDD.collect();
			System.out.println("------Contents of filesRDD------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2.trim() + ")");
			}
			System.out.println("--------------------------------");
		}
		
		/* 
		 * Initial Job
		 * Creates initial JavaPairRDD from filesRDD
		 * Contains each word@document from the corpus and also attaches the document size for 
		 * later use
		 * 
		 * Input:  ( filePath , fileContents )
		 * Map:    ( (word@document) , docSize )
		 */
		JavaPairRDD<String,Integer> wordsRDD = filesRDD.flatMapToPair(
			new PairFlatMapFunction<Tuple2<String,String>,String,Integer>() {
				public Iterable<Tuple2<String,Integer>> call(Tuple2<String,String> x) {
					// Collect data attributes
					String[] filePath = x._1.split("/");
					for(String str: filePath){
						//System.out.println("Abhash Jain: "+ str);
					}
					String document = filePath[filePath.length-1];
					String fileContents = x._2;
					//System.out.println("File Content is " + fileContents);
					String[] words = fileContents.split("\\s+");
					int docSize = words.length;
					
					// Output to Arraylist
					ArrayList ret = new ArrayList();
					for(String word : words) {
						ret.add(new Tuple2(word.trim() + "@" + document, docSize));
					}
					return ret;
				}
			}
		);
		
		//Print wordsRDD contents
		if (DEBUG) {
			List<Tuple2<String, Integer>> list = wordsRDD.collect();
			System.out.println("------Contents of wordsRDD------");
			for (Tuple2<String, Integer> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}		
		
		
		/* 
		 * TF Job (Word Count Job + Document Size Job)
		 * Gathers all data needed for TF calculation from wordsRDD
		 *
		 * Input:  ( (word@document) , docSize )
		 * Map:    ( (word@document) , (1/docSize) )
		 * Reduce: ( (word@document) , (wordCount/docSize) )
		 */

		//Reduce lambda function for TF Job
		Function2<String,String,String> tfReduce = new Function2<String,String,String> (){
			@Override
			public String call(String a,String b) {
				//split the string to get doc size from two values received as argument
				String first[] = a.split("/");
				String second[] = b.split("/");
				//combined the result to word count
				int combinedValue = Integer.parseInt(first[0]) + Integer.parseInt(second[0]);
				String finalString = Integer.toString(combinedValue) + "/" + first[1];
				return finalString;
			}
		};

		JavaPairRDD<String,String> tfRDD = wordsRDD.flatMapToPair(
			/************ YOUR CODE HERE ************/
			//Map task for TF job
			new PairFlatMapFunction<Tuple2<String,Integer>,String,String>() {
				public Iterable<Tuple2<String,String>> call(Tuple2<String,Integer> x) {
					//return an list contains the output as 1/docSize
					ArrayList ret = new ArrayList();
					ret.add(new Tuple2(x._1,"1/"+ Integer.toString(x._2)));
					return ret;
				}
			}
		).reduceByKey(
			/************ YOUR CODE HERE ************/
			//calls the TF lambda function
			tfReduce
		);
		
		//Print tfRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = tfRDD.collect();
			System.out.println("-------Contents of tfRDD--------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}
		

		/*
		 * IDF Job
		 * Gathers all data needed for IDF calculation from tfRDD
		 *
		 * Input:  ( (word@document) , (wordCount/docSize) )
		 * Map:    ( word , (1/document) )
		 * Reduce: ( word , (numDocsWithWord/document1,document2...) )
		 * Map:    ( (word@document) , (numDocs/numDocsWithWord) )
		 */
		//Reduce Lambda Function for IDF job
		Function2<String,String,String> idfReduce = new Function2<String,String,String> (){
			@Override
			public String call(String a,String b) {
				//split the two value to get the document name
				String s1[] = a.split("/");
				String s2[] = b.split("/");
				//set to store the unique document name
				Set<String> wordSet = new HashSet<>();
				wordSet.add(s1[1]);
				wordSet.add(s2[1]);
				//create a final string as mentioned in the final outcome of reduce job and return it
				String finalString = wordSet.size()+"/";
				for(String s: wordSet){
					finalString = finalString  +s +",";
				}
				return finalString.substring(0,finalString.length()-1);
			}
		};
		JavaPairRDD<String,String> idfRDD = tfRDD.flatMapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFlatMapFunction<Tuple2<String,String>,String,String>() {
				public Iterable<Tuple2<String,String>> call(Tuple2<String,String> x) {
					//Map function to return the IDF  job and emit output as "1/documentname"
					String s[] = x._1.split("@");
					ArrayList ret = new ArrayList();
					ret.add(new Tuple2(s[0],"1/"+ s[1]));
					return ret;
				}
			}
		).reduceByKey(
			/************ YOUR CODE HERE ************/
			//Calls the IDF labda function to reduce
			idfReduce
			
		).flatMapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFlatMapFunction<Tuple2<String,String>,String,String>() {
				public Iterable<Tuple2<String,String>> call(Tuple2<String,String> x) {
					//emits the document in ( (word@document) , (numDocs/numDocsWithWord) )
					//numsDocs is already calculated which has number of documents
					String s1[] = x._2.split("/");
					String docsWord[] = s1[1].split(",");
					String numDocsStr = Long.toString(numDocs);
					ArrayList ret = new ArrayList();
					for(String doc: docsWord){
						ret.add(new Tuple2(x._1+"@"+doc,numDocsStr + "/" + s1[0]));
					}
					return ret;
				}
			}
		);
		
		//Print idfRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = idfRDD.collect();
			System.out.println("-------Contents of idfRDD-------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}
	
		/*
		 * TF * IDF Job
		 * Calculates final TFIDF value from tfRDD and idfRDD
		 *
		 * Input:  ( (word@document) , (wordCount/docSize) )          [from tfRDD]
		 * Map:    ( (word@document) , TF )
		 * 
		 * Input:  ( (word@document) , (numDocs/numDocsWithWord) )    [from idfRDD]
		 * Map:    ( (word@document) , IDF )
		 * 
		 * Union:  ( (word@document) , TF )  U  ( (word@document) , IDF )
		 * Reduce: ( (word@document) , TFIDF )
		 * Map:    ( (document@word) , TFIDF )
		 *
		 * where TF    = wordCount/docSize
		 * where IDF   = ln(numDocs/numDocsWithWord)
		 * where TFIDF = TF * IDF
		 */

		 //Lambda function for TFIDF reduce
		Function2<Double,Double,Double> tfidfReduce = new Function2<Double,Double,Double> (){
			@Override
			public Double call(Double a,Double b) {
				//get the TF and IDF and returns TFIDF 
				return (a*b);
			}
		};

		JavaPairRDD<String,Double> tfFinalRDD = tfRDD.mapToPair(
			new PairFunction<Tuple2<String,String>,String,Double>() {
				public Tuple2<String,Double> call(Tuple2<String,String> x) {
					double wordCount = Double.parseDouble(x._2.split("/")[0]);
					double docSize = Double.parseDouble(x._2.split("/")[1]);
					double TF = wordCount/docSize;
					return new Tuple2(x._1, TF);
				}
			}
		);
		
		JavaPairRDD<String,Double> idfFinalRDD = idfRDD.mapToPair(
			
			/************ YOUR CODE HERE ************/
			//function to precomput map from string expression to double
			new PairFunction<Tuple2<String,String>,String,Double>() {
				public Tuple2<String,Double> call(Tuple2<String,String> x) {
					double numDocs = Double.parseDouble(x._2.split("/")[0]);
					double numDocsWithWord = Double.parseDouble(x._2.split("/")[1]);
					double idf = Math.log((double)numDocs/numDocsWithWord);
					return new Tuple2(x._1, idf);
				}
			}
		);
		
		JavaPairRDD<String,Double> tfidfRDD = tfFinalRDD.union(idfFinalRDD).reduceByKey(
			
			/************ YOUR CODE HERE ************/
			//calls the TFIDF lamda function and which return the word@document : TFIDF
			tfidfReduce
		).flatMapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFlatMapFunction<Tuple2<String,Double>,String,Double>() {
				public Iterable<Tuple2<String,Double>> call(Tuple2<String,Double> x) {
					//emits the document@word with TFIDF
					//nothing just formatting the key.
					String s1[] = x._1.split("@");
					ArrayList ret = new ArrayList();
					ret.add(new Tuple2(s1[1]+"@"+s1[0],x._2));
					return ret;
				}
			}
		);
		
		//Print tfidfRDD contents in sorted order
		Map<String, Double> sortedMap = new TreeMap<>();
		List<Tuple2<String, Double>> list = tfidfRDD.collect();
		for (Tuple2<String, Double> tuple : list) {
			sortedMap.put(tuple._1, tuple._2);
		}
		if(DEBUG) System.out.println("-------Contents of tfidfRDD-------");
		for (String key : sortedMap.keySet()) {
			System.out.println(key + "\t" + sortedMap.get(key));
		}
		if(DEBUG) System.out.println("--------------------------------");	 
	}	
}