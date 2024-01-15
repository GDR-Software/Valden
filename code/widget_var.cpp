#include "gln.h"
#include "widget_var.h"

CEditorWidgetVar::CEditorWidgetVar( void )
{
    m_pLabel = NULL;
    memset( m_pBuffer, 0, sizeof(m_pBuffer) );
    m_Integer = 0;
    m_Float = 0;
}

CEditorWidgetVar::~CEditorWidgetVar()
{
    if ( m_pLabel ) {
        delete[] m_pLabel;
    }
}

void CEditorWidgetVar::SetLabel( const char *label )
{
    uint64_t len;

    if ( m_pLabel ) {
        delete[] m_pLabel;
    }

    len = strlen( label );
    m_pLabel = new char[len + 1];
    N_strncpyz( m_pLabel, label, len + 1 );
}

void CEditorWidgetVar::SetType( widgetVarType_t type )
{
    m_Type = type;
}

void CEditorWidgetVar::PollInput( void )
{
    switch ( m_Type ) {
    case WVT_INT:
        break;
    };
}
