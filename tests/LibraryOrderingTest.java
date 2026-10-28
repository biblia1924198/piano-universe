package com.luci.pianoroom;
import java.util.*;
public final class LibraryOrderingTest {
 static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
 public static void main(String[]args){
  String[] names={"Zeta","Álbum","amarelo","Bösendorfer","alfa","ALFA"};String[] keys={"z.sf2","a.sf2","am.sf2","b.sf2","later.sf2","earlier.sf2"};
  String[] originals=names.clone();int[] current={0,3,2,1};int[] selected=current.clone();int[] order=LibraryOrdering.indices(names,keys);
  check(Arrays.equals(order,new int[]{1,5,4,2,3,0}),"Portuguese A-Z ignores case/accents and breaks ties by stable file key");
  check(Arrays.equals(names,originals)&&Arrays.equals(current,selected),"sorting does not mutate bank storage or channel selections");
  for(int position=0;position<order.length;position++)check(keys[order[position]].equals(keys[new int[]{1,5,4,2,3,0}[position]]),"visible position resolves to original sampler identity");
  check(LibraryOrdering.comparator().compare("ÁGUA","agua")==0,"accents/case have equal alphabetic weight");
  String[] renamed=names.clone();renamed[0]="Abeto";int[] changed=LibraryOrdering.indices(renamed,keys);check(changed[0]==0&&keys[changed[0]].equals("z.sf2")&&selected[0]==0,"rename repositions only the view, while selected bank keeps same key/index");
  check(LibraryOrdering.indices(new String[0],new String[0]).length==0,"empty library list");
  check(Arrays.equals(LibraryOrdering.indices(new String[]{"Piano","Piano","Piano"},new String[]{"3","1","2"}),new int[]{1,2,0}),"duplicate display names retain distinct stable identities");
  try{LibraryOrdering.indices(new String[]{"a"},new String[0]);throw new AssertionError("mismatched metadata accepted");}catch(IllegalArgumentException expected){}
  System.out.println("PASS: Portuguese A-Z, accents/case, duplicate names, stable identities, rename reposition, empty catalog, and unchanged channel selections.");
 }
}
