#include "kspread_corba_util.h"
#include "kspread_util.h"
#include "kspread_map.h"

KSpread::Cell util_parseCell( const QString& _str )
{
  KSpreadPoint c( _str );
  if ( !c.isValid() )
  {
    KSpread::MalformedExpression exc;
    exc.expr = CORBA::string_dup( _str );
    mico_throw( exc );
  }

  KSpread::Cell c2;
  c2.x = c.pos.x();
  c2.y = c.pos.y();
  c2.table = CORBA::string_dup( "" );
  
  return c2;
}

KSpread::Cell util_parseCell( const QString& _str, KSpreadMap* _map )
{
  KSpreadPoint c( _str, _map );

  if ( !c.table )
  {
    KSpread::UnknownTable exc;
    exc.table = CORBA::string_dup( c.tableName );
    mico_throw( exc );
  }

  if ( !c.isValid() )
  {
    KSpread::MalformedExpression exc;
    exc.expr = CORBA::string_dup( _str );
    mico_throw( exc );
  }

  KSpread::Cell c2;
  c2.x = c.pos.x();
  c2.y = c.pos.y();
  c2.table = CORBA::string_dup( c.tableName );
  
  return c2;
}

KSpread::Range util_parseRange( const QString& _str )
{
  KSpreadRange r( _str );
  
  if ( !r.isValid() )
  {
    KSpread::MalformedExpression exc;
    exc.expr = CORBA::string_dup( _str );
    mico_throw( exc );
  }

  KSpread::Range r2;
  r2.left = r.range.left();
  r2.right = r.range.right();
  r2.top = r.range.top();
  r2.bottom = r.range.bottom();
  r2.table = CORBA::string_dup( "" );
  
  return r2;
}

KSpread::Range util_parseRange( const QString& _str, KSpreadMap* _map )
{
  KSpreadRange r( _str );

  if ( !r.table )
  {
    KSpread::UnknownTable exc;
    exc.table = CORBA::string_dup( r.tableName );
    mico_throw( exc );
  }
  
  if ( !r.isValid() )
  {
    KSpread::MalformedExpression exc;
    exc.expr = CORBA::string_dup( _str );
    mico_throw( exc );
  }

  KSpread::Range r2;
  r2.left = r.range.left();
  r2.right = r.range.right();
  r2.top = r.range.top();
  r2.bottom = r.range.bottom();
  r2.table = CORBA::string_dup( r.tableName );
  
  return r2;
}
