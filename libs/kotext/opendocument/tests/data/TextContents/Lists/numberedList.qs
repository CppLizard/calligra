include("common.qs");

var listFormat = QTextListFormat.clone(defaultListFormat);
listFormat.setStyle(QTextListFormat.ListDecimal);
setFormatProperty(listFormat, KoListStyle.ListItemSuffix, ".");
setFormatProperty(listFormat, KoListStyle.StartValue, 1);
setFormatProperty(listFormat, KoListStyle.RelativeBulletSize,100);
setFormatProperty(listFormat, KoListStyle.AlignmentMode,false);
cursor.createList(listFormat);
cursor.insertText("This is an example of numbered list.", defaultListItemFormat);
cursor.insertBlock();
cursor.insertText("This is an example of numbered list.", defaultListItemFormat);
cursor.insertBlock();
cursor.insertText("This is an example of numbered list.", defaultListItemFormat);
cursor.insertBlock();
cursor.insertText("This is an example of numbered list.", defaultListItemFormat);

document;
