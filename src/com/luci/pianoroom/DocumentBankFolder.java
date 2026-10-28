package com.luci.pianoroom;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import java.io.*;
import java.util.HashSet;

/** Bounded SAF directory copy on the importer worker; no broad storage permission. */
final class DocumentBankFolder {
 private final ContentResolver resolver;private final Uri tree;private final File root;
 private long total;private int entries;private final HashSet<String> seen=new HashSet<>();
 private DocumentBankFolder(ContentResolver resolver,Uri tree,File root){this.resolver=resolver;this.tree=tree;this.root=root;}
 static void copy(ContentResolver resolver,Uri tree,File root)throws IOException {
  if(!root.mkdirs())throw new IOException("Não criou pasta temporária SFZ");
  new DocumentBankFolder(resolver,tree,root).directory(DocumentsContract.getTreeDocumentId(tree),root,0);
 }
 private void directory(String id,File folder,int depth)throws IOException {
  if(depth>32||!seen.add(id))throw new IOException("Pasta recursiva ou profunda demais");
  String[] columns={DocumentsContract.Document.COLUMN_DOCUMENT_ID,DocumentsContract.Document.COLUMN_DISPLAY_NAME,DocumentsContract.Document.COLUMN_MIME_TYPE,DocumentsContract.Document.COLUMN_SIZE};
  try(Cursor cursor=resolver.query(DocumentsContract.buildChildDocumentsUriUsingTree(tree,id),columns,null,null,null)){
   if(cursor==null)throw new IOException("Pasta inacessível");
   while(cursor.moveToNext()){
    if(++entries>10000)throw new IOException("Pasta tem mais de 10 mil arquivos");
    String childId=cursor.getString(0),name=cursor.getString(1),type=cursor.getString(2);
    if(name==null||name.isEmpty()||name.equals(".")||name.equals("..")||name.contains("/")||name.contains("\\"))throw new IOException("Nome de arquivo inválido");
    File target=SfzBank.safe(root,root.toPath().relativize(folder.toPath().resolve(name)).toString());
    if(target.exists())throw new IOException("Nome duplicado na pasta: "+name);
    if(DocumentsContract.Document.MIME_TYPE_DIR.equals(type)){if(!target.mkdir())throw new IOException("Pasta indisponível");directory(childId,target,depth+1);continue;}
    long length=cursor.isNull(3)?0:cursor.getLong(3);if(length>0){SfzBank.accumulate(total,length,SfzBank.MAX_SOURCE_BYTES);SfzBank.space(root,length);}
    try(InputStream in=resolver.openInputStream(DocumentsContract.buildDocumentUriUsingTree(tree,childId));OutputStream out=new FileOutputStream(target)){
     if(in==null)throw new IOException("Arquivo inacessível: "+name);byte[] b=new byte[65536];int n;
     while((n=in.read(b))!=-1){total=SfzBank.accumulate(total,n,SfzBank.MAX_SOURCE_BYTES);if((total&0x3ffffffL)<n)SfzBank.space(root,64L*1024*1024);out.write(b,0,n);}
    }
   }
  }catch(SecurityException|IllegalArgumentException e){throw new IOException("Sem acesso à pasta SFZ: "+e.getMessage(),e);}
 }
}
