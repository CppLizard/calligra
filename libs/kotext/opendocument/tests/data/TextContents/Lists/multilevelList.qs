include("common.qs");

var listFormat1 = QTextListFormat.clone(defaultListFormat);
setFormatProperty(listFormat1, QTextFormat.ListStyle, KoListStyle.BlackCircle);
setFormatProperty(listFormat1, KoListStyle.BulletCharacter, 0x25CF);
setFormatProperty(listFormat1, KoListStyle.MinimumWidth, 18);
setFormatProperty(listFormat1, KoListStyle.Indent, 18.);
setFormatProperty(listFormat1, KoListStyle.RelativeBulletSize,45);
setFormatProperty(listFormat1, KoListStyle.AlignmentMode,false);

var unnumberedFormat = QTextBlockFormat.clone(defaultBlockFormat);
setFormatProperty(unnumberedFormat, KoParagraphStyle.UnnumberedListItem, 1);

var list1 = cursor.createList(listFormat1);
cursor.insertText("Level 1 item", defaultListItemFormat);
cursor.insertBlock(unnumberedFormat);
list1.add(cursor.block());
cursor.insertText("Level 1 Unnumbered", defaultListItemFormat);

var listFormat2 = QTextListFormat.clone(listFormat1);
setFormatProperty(listFormat2, KoListStyle.Level, 2);
setFormatProperty(listFormat2, KoListStyle.Indent, 54.);
cursor.insertBlock(defaultBlockFormat);
cursor.insertText("Level 2 item", defaultListItemFormat);
var list2 = cursor.createList(listFormat2);
cursor.insertBlock(unnumberedFormat);
list2.add(cursor.block());
cursor.insertText("Level 2 Unnumbered", defaultListItemFormat);
cursor.insertBlock(defaultBlockFormat);
list2.add(cursor.block());
cursor.insertText("Level 2 item", defaultListItemFormat);

cursor.insertBlock();
cursor.insertText("Level 1 item", defaultListItemFormat);
list1.add(cursor.block());

document;
