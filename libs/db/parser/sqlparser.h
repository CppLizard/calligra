#ifndef _SQLPARSER_H_
#define _SQLPARSER_H_
#include <db/field.h>
#include "parser.h"
#include "sqltypes.h"

bool parseData(KexiDB::Parser *p, const char *data);
const char* tokenName(unsigned int offset);
unsigned int maxToken();
/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_SQLPARSER_TAB_H_INCLUDED
# define YY_YY_SQLPARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SQL_TYPE = 258,
    AS = 259,
    ASC = 260,
    AUTO_INCREMENT = 261,
    BIT = 262,
    BITWISE_SHIFT_LEFT = 263,
    BITWISE_SHIFT_RIGHT = 264,
    BY = 265,
    CHARACTER_STRING_LITERAL = 266,
    CONCATENATION = 267,
    CREATE = 268,
    DESC = 269,
    DISTINCT = 270,
    DOUBLE_QUOTED_STRING = 271,
    FROM = 272,
    HEX_LITERAL = 273,
    JOIN = 274,
    KEY = 275,
    LEFT = 276,
    LESS_OR_EQUAL = 277,
    SQL_NULL = 278,
    SQL_IS = 279,
    SQL_IS_NULL = 280,
    SQL_IS_NOT_NULL = 281,
    ORDER = 282,
    PRIMARY = 283,
    SELECT = 284,
    INTEGER_CONST = 285,
    REAL_CONST = 286,
    RIGHT = 287,
    SQL_ON = 288,
    DATE_CONST = 289,
    DATETIME_CONST = 290,
    TIME_CONST = 291,
    TABLE = 292,
    IDENTIFIER = 293,
    IDENTIFIER_DOT_ASTERISK = 294,
    QUERY_PARAMETER = 295,
    VARCHAR = 296,
    WHERE = 297,
    SCAN_ERROR = 298,
    UNION = 299,
    EXCEPT = 300,
    INTERSECT = 301,
    OR = 302,
    AND = 303,
    XOR = 304,
    NOT = 305,
    GREATER_OR_EQUAL = 306,
    NOT_EQUAL = 307,
    NOT_EQUAL2 = 308,
    SQL_IN = 309,
    LIKE = 310,
    NOT_LIKE = 311,
    ILIKE = 312,
    SIMILAR_TO = 313,
    NOT_SIMILAR_TO = 314,
    SIMILAR = 315,
    BETWEEN = 316,
    NOT_BETWEEN = 317,
    UMINUS = 318
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 492 "sqlparser.y" /* yacc.c:1909  */

    QString* stringValue;
    QByteArray* binaryValue;
    qint64 integerValue;
    bool booleanValue;
    KexiDB::Field::Type colType;
    KexiDB::Field *field;
    KexiDB::BaseExpr *expr;
    KexiDB::NArgExpr *exprList;
    KexiDB::ConstExpr *constExpr;
    KexiDB::QuerySchema *querySchema;
    SelectOptionsInternal *selectOptions;
    OrderByColumnInternal::List *orderByColumns;
    QVariant *variantValue;

#line 134 "sqlparser.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SQLPARSER_TAB_H_INCLUDED  */
#endif
