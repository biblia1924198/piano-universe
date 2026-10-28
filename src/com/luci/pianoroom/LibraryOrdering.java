package com.luci.pianoroom;
import java.text.Collator;
import java.util.*;
/** Display order only: persistent IDs and sampler indices never change. */
public final class LibraryOrdering {
 private LibraryOrdering(){}
 public static Comparator<String> comparator(){final Collator collator=Collator.getInstance(new Locale("pt","BR"));collator.setStrength(Collator.PRIMARY);return (a,b)->collator.compare(a==null?"":a,b==null?"":b);}
 public static int[] indices(String[] names,String[] keys){if(names.length!=keys.length)throw new IllegalArgumentException("names/keys");final Comparator<String> alphabet=comparator();Integer[] positions=new Integer[names.length];for(int i=0;i<positions.length;i++)positions[i]=i;Arrays.sort(positions,(a,b)->{int order=alphabet.compare(names[a],names[b]);if(order!=0)return order;order=keys[a].compareTo(keys[b]);return order!=0?order:Integer.compare(a,b);});int[] order=new int[positions.length];for(int i=0;i<order.length;i++)order[i]=positions[i];return order;}
}
