package com.starthum;

import java.util.ArrayList;
import java.util.HashMap;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import com.starthum.util.LazyAdapter;
import com.starthum.util.XMLParser;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.TextView;

public class ShowResults extends Activity {
	
	// XML node keys
	static String KEY_SONG = "song"; // parent node
	static public String KEY_TITLE = "title";
	static public String KEY_ARTIST = "author";
	static public String KEY_GENRE = "genre";
	static public String KEY_THUMB_URL = "thumb_url";
	static public String KEY_POSITION = "position";
	
	ListView list;
	LazyAdapter adapter;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_show_results);
		
		ArrayList<HashMap<String, String>> songsList = new ArrayList<HashMap<String, String>>();

		XMLParser parser = new XMLParser();
		Document doc = parser.getDomElement(QueryByHumming.RESULT_FILE); // getting DOM element

		NodeList nl = doc.getElementsByTagName(KEY_SONG);
		TextView t = (TextView)findViewById(R.id.results);
		t.setText(String.valueOf(QueryByHumming.t1 - QueryByHumming.t0));
		int i;
		for(i = 0; i < nl.getLength(); i++) {
			// creating new HashMap
			HashMap<String, String> map = new HashMap<String, String>();
			Element e = (Element) nl.item(i);
			// adding each child node to HashMap key > value
			map.put(KEY_TITLE, parser.getValue(e, KEY_TITLE));
			map.put(KEY_ARTIST, parser.getValue(e, KEY_ARTIST));
			map.put(KEY_GENRE, parser.getValue(e, KEY_GENRE));
			map.put(KEY_POSITION, Integer.toString(i+1));
			map.put(KEY_THUMB_URL, parser.getValue(e, KEY_THUMB_URL));

			// adding HashList to ArrayList
			songsList.add(map);
		}
		
		list = (ListView)findViewById(R.id.list);

		// Getting adapter by passing xml data ArrayList
		adapter = new LazyAdapter(this, songsList);
		list.setAdapter(adapter);

		// Click event for single list row
		list.setOnItemClickListener(new OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> parent, View view,
				int position, long id) {

			}
		});
		
	}
}