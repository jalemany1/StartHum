package com.starthum;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.util.Timer;
import java.util.TimerTask;

import com.starthum.util.ExtAudioRecorder;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings.Secure;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Button;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 *
 * @see SystemUiHider
 */
public class QueryByHumming extends Activity {
    
    protected static String HOST_NAME = "192.168.1.12";
    protected static int PORT = 7000;
    public static String APP_FOLDER_PATH;
    public static String HUMMING_FILE;
    public static String RESULT_FILE;
    
    private ExtAudioRecorder recorder;
    private Button startHum;
    
    public static double t0, t1;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_fullscreen);

        // update the directory files
        APP_FOLDER_PATH = appFolderPath();
        HUMMING_FILE = APP_FOLDER_PATH + "/" + "humming.wav";
        RESULT_FILE = APP_FOLDER_PATH + "/" + "result.xml";
        
        // button functionalities
        final Animation animScale = AnimationUtils.loadAnimation(this, R.anim.scale);
        startHum = (Button) findViewById (R.id.start);
        startHum.setOnClickListener(new OnClickListener() {
    		@Override
    		public void onClick(View v) {
    			// change state
    	    	startHum.setEnabled(false);
    	    	startHum.setText(R.string.hum_button);
    			v.startAnimation(animScale);
    			
    			start(v);
    			
    			Timer buttonEnable = new Timer();
    			buttonEnable.schedule(new TimerTask() {

    				@Override
    				public void run() {
    					runOnUiThread(new Runnable() {

    						@Override
    						public void run() {
    							recorder.stop();
    			    	    	recorder.release();
    			    	    	recorder = null;
    			    	    	connect();
    						}
    					});
    				}
    			}, 15 * 1000);
    			
    			Timer buttonNext = new Timer();
    			buttonNext.schedule(new TimerTask() {

    				@Override
    				public void run() {
    					runOnUiThread(new Runnable() {

    						@Override
    						public void run() {
    							startHum.setText("");
    							startHum.clearAnimation();
    							startHum.setEnabled(true);
    						}
    					});
    				}
    			}, 20 * 1000);
    		}
          });
        
    }
    
    private String appFolderPath(){
        String filepath = Environment.getExternalStorageDirectory().getPath();
        File file = new File(filepath, "StartHum");
        
        if (!file.exists()) file.mkdirs();
        
        return file.getAbsolutePath();
    }

    public void connect() {
    	Thread streamThread = new Thread(new Runnable() {

            @Override
            public void run() {
            	t0 = System.currentTimeMillis();
            	Socket session = null;
            	DataOutputStream dos = null;
            	DataInputStream dis = null;
            	
            	try {
            		session = new Socket(InetAddress.getByName(HOST_NAME), PORT);
            	} catch (Exception e) {
            		e.printStackTrace();
            	}
            	// send ANDROID_ID
            	try {
            		dos = new DataOutputStream(session.getOutputStream());
            		dos.writeUTF(Secure.getString(getContentResolver(), Secure.ANDROID_ID)); 
            		dos.flush();
            	} catch (Exception e) {
            		e.printStackTrace();
            	}
            	// send MIC recorder
            	try {
            		BufferedOutputStream bos = new BufferedOutputStream(session.getOutputStream());
            		File transferFile = new File(HUMMING_FILE);
            		if (transferFile.isFile()) {
                        FileInputStream fis = new FileInputStream(transferFile);
                        // send size
                        dos.writeInt((int) transferFile.length());
                        dos.flush();
                        // send file
                        int out;
                        byte[] buf = new byte[1024];
                        while ((out = fis.read(buf, 0, 1024)) != -1) {
                            bos.write(buf, 0, out);
                            bos.flush();
                        }
                        fis.close();
                    }
            		transferFile.delete();
            	} catch (Exception e) {
            		e.printStackTrace();
            	}
            	// receive XML result
            	try {
            		dis = new DataInputStream(session.getInputStream());
            		int size = dis.readInt();
                    byte[] buf = new byte[1024];
            		BufferedInputStream bis = new BufferedInputStream(session.getInputStream());
            		FileOutputStream fos = new FileOutputStream(RESULT_FILE);
                    for (int i = 0; i < size;) {
                        int in = bis.read(buf);
                        fos.write(buf, 0, in);
                        i += in;
                    }
                    fos.close();
                    session.close();
            	} catch (Exception e) {
            		e.printStackTrace();
            	}
            	t1 = System.currentTimeMillis();
            	Intent intent = new Intent(QueryByHumming.this, ShowResults.class);
				startActivity(intent);
            }
    	});
    	
    	streamThread.start();
    }
    
	public void start(View view) {
    	
    	recorder = ExtAudioRecorder.getInstanse(false);
    	recorder.setOutputFile(HUMMING_FILE);
    	
    	try {
    		recorder.prepare();
    	} catch (Exception e) {
    		e.printStackTrace();
    	}

    	recorder.start();
    }
	
	@Override
	protected void onDestroy() {
		if (recorder != null) {
			recorder.stop();
			recorder.release();
		}
		super.onDestroy();
	}
}
