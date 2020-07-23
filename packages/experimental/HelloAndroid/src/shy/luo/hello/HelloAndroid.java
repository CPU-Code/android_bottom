/*
 * @Author: cpu_code
 * @Date: 2020-07-22 20:54:53
 * @LastEditTime: 2020-07-22 21:54:27
 * @FilePath: \android_bottom\packages\experimental\HelloAndroid\src\shy\luo\hello\HelloAndroid.java
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
package shy.luo.hello;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class HelloAndroid extends Activity 
{
	// 定义了一个Activity组件HelloAndroid
	private final static String LOG_TAG = "shy.luo.hello.HelloAndroid";
   
	@Override
		public void onCreate(Bundle savedInstanceState) 
		{
        	super.onCreate(savedInstanceState);
        	setContentView(R.layout.main);
			
        	Log.i(LOG_TAG, "HelloAndroid Activity Created.");
    	}
}
