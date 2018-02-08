/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfLpad.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfLpad;

extern mtdModule mtdInteger;

extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;

static mtcName mtfLpadFunctionName[1] = {
    { NULL, 4, (void*)"LPAD" }
};

static IDE_RC mtfLpadEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfLpad = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (�� �����ڰ� �ƴ�)
    mtfLpadFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLpadEstimate
};

static IDE_RC mtfLpadCalculateFor2Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfLpadCalculateFor3Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfLpadCalculateNcharFor2Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfLpadCalculateNcharFor3Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLpadCalculateFor2Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLpadCalculateFor3Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLpadCalculateNcharFor2Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLpadCalculateNcharFor3Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLpadEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    const mtdModule * sModules[3];
    const mtlModule * sLanguage;

    SInt              i;
    mtcNode         * sLengthNode;
    mtcColumn       * sLengthColumn;
    mtcNode         * sPadCharNode;
    mtcColumn       * sPadCharColumn;
    UChar           * sPadIndex;
    UChar           * sPadFence;
    mtdIntegerType    sPrecision;
    mtdIntegerType    sPrecision2;
    mtdIntegerType    sCount = 0;
    void*             sValueTemp;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );
    sModules[1] = &mtdInteger;
    sModules[2] = sModules[0];

    if( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
        == MTC_NODE_REESTIMATE_FALSE )
    {
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        // PROJ-1579 NCHAR
        if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
            (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
        {
            if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor3Args;
            }

            if( aStack[1].column->language->id == MTL_UTF16_ID )
            {
                sPrecision = (UShort)idlOS::floor(4000 / MTL_UTF16_PRECISION);
            }
            else
            {
                sPrecision = (UShort)idlOS::floor(4000 / MTL_UTF8_PRECISION);
            }

            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             &mtdNvarchar,
                                             1,
                                             sPrecision,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor3Args;
            }

            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             &mtdVarchar, // fix BUG-19838
                                             1,
                                             4000,
                                             0 )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // LPAD( 123, 10, '0' ) or
        // LPAD( 123, 10+1, '0' ) or
        // LPAD( 123, to_number('10' ) �� ����
        // validation �ܰ迡�� constant expression�� ����Ǿ�
        // length ���� �� �� �ִ� ����
        // LPAD �÷��� precision�� ���� ���������� �����Ѵ�.
        //
        //  LPAD
        //   |
        //  [ 123 ] - [ 10 ] - [ '0' ]
        //           ^^^^^^^^
        //           sLengthNode

        sLengthNode = mtf::convertedNode( aNode->arguments->next,
                                          aTemplate );

        if( ( sLengthNode == aNode->arguments->next ) &&
            ( ( aTemplate->rows[sLengthNode->table].lflag & MTC_TUPLE_TYPE_MASK )
              == MTC_TUPLE_TYPE_CONSTANT ) )
        {
            sLengthColumn = &(aTemplate->rows[sLengthNode->table].
                              columns[sLengthNode->column]);

            sValueTemp = (void*)mtd::valueForModule(
                                     (smiColumn*)sLengthColumn,
                                     aTemplate->rows[sLengthNode->table].row,
                                     MTD_OFFSET_USE,
                                     sLengthColumn->module->staticNull );

            if( sLengthColumn->module->isNull(
                sLengthColumn,
                sValueTemp ) == ID_TRUE )
            {
                sPrecision = 0;
            }
            else
            {
                sPrecision = *(mtdIntegerType *) sValueTemp;
            }

            // ������ ���� sPrecision�� ���ڰ����� �ǹ�
            // sPrecision�� �� language���� �ִ� ����ũ�⸦ ���Ѵ�.

            if( sPrecision < 0 )
            {
                sPrecision = 0;
            }
            else
            {
                // lpad( 123, 10, '0' ) �� ���
                // 123�� language�� ������.

                // PROJ-1579 NCHAR
                if( (sModules[0]->id == MTD_NCHAR_ID) ||
                    (sModules[0]->id == MTD_NVARCHAR_ID) )
                {
                    // Nothing to do
                    // ���� ���� �״�� precision�� �ȴ�.
                }
                else
                {
                    /* BUG-34165 
                     * RPAD( A, 20, 'f' ) �� ���� 3��° ���ڰ� �ְ�, 
                     * validation �ܰ迡�� ���� �� �� �ִ� ��� 
                     * �� �� ��Ȯ�� precision�� ����ϵ��� ����. */ 

                    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
                    {
                        sPadCharNode = mtf::convertedNode( aNode->arguments->next->next,
                                aTemplate );

                        if( ( sPadCharNode == aNode->arguments->next->next )       &&
                            ( ( aTemplate->rows[sPadCharNode->table].lflag & MTC_TUPLE_TYPE_MASK ) 
                             == MTC_TUPLE_TYPE_CONSTANT ) )
                        {
                            /* sCount�� �е����ڰ� �ִ�� �� �� �ִ� ���� ��Ÿ����. */
                            sCount = (mtdIntegerType) idlOS::ceil( (double) aStack[1].column->precision / 
                                     aStack[1].column->language->maxPrecision(1) );

                            sCount = sPrecision - sCount;

                            sPadCharColumn = &(aTemplate->rows[sPadCharNode->table].
                                    columns[sPadCharNode->column]);

                            sValueTemp = (void*) mtd::valueForModule(
                                    (smiColumn*)sPadCharColumn,
                                    aTemplate->rows[sPadCharNode->table].row,
                                    MTD_OFFSET_USE,
                                    sPadCharColumn->module->staticNull );

                            sPadIndex = (UChar*) ((mtdCharType*) sValueTemp)->value;
                            sPadFence = sPadIndex + ((mtdCharType*)sValueTemp)->length;
                            sPrecision2 = aStack[1].column->precision;
                            sLanguage = sPadCharColumn->language;

                            /* BUG-36354 valgrind warning lpad('abc',10,'') */
                            if( sPadCharColumn->module->isNull( sPadCharColumn, sValueTemp ) != ID_TRUE )
                            {
                                /* �е����ڰ� �ִ�� ���� ��쿡 �� ����Ʈ�� �� �ʿ������� ��� */
                                for( i = 0 ; i < sCount ; i++ )
                                {
                                    sPrecision2 += mtl::getOneCharSize( sPadIndex, 
                                                                        sPadFence, 
                                                                        sLanguage );

                                    (void)sLanguage->nextCharPtr( &sPadIndex, sPadFence );

                                    if ( sPadIndex >= sPadFence )
                                    {
                                        sPadIndex = (UChar*) ((mtdCharType*) sValueTemp)->value;
                                    }
                                    else
                                    {
                                        // Nothing to do
                                    }
                                }
                            }
                            else
                            {
                                /* Nothing To Do */
                            }
                        }
                        else
                        {
                            // Nothing to do
                        }
                    }
                    else
                    {
                        // Nothing to do
                    }

                    sPrecision =
                        aStack[1].column->language->maxPrecision( sPrecision );

                    if ( sCount > 0 )
                    {
                        sPrecision = IDL_MIN( sPrecision, sPrecision2 );
                    }
                    else
                    {
                        // Nothing to do
                    }
                }
            }

            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             sModules[0],
                                             1,
                                             sPrecision,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    // fix BUG-13830
    // �� : LPAD( 123, 10, '0' ) or LPAD( 123, 10+1, '0' )�� ����
    // length �� 
    // validation�ܰ迡�� constant expression�� ����Ǵ� ����
    // LPAD �� ��������÷��� precision�� length ���̷� ����Ѵ�.
    if( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
        &&
        ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
              & MTC_TUPLE_TYPE_MASK )
            == MTC_TUPLE_TYPE_CONSTANT ) ||
          ( ( aTemplate->rows[aNode->arguments->next->table].lflag
              & MTC_TUPLE_TYPE_MASK )
            == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            
        // BUG-38070 undef type���� re-estimate���� �ʴ´�.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLeftPad( const mtlModule * aLanguage,
                   UChar           * aResult,
                   UShort          * aResultLen,
                   UChar           * aSource,
                   UShort            aSourceLen,
                   SInt              aLength,
                   UChar           * aPad,
                   UShort            aPadLen )
{
/***********************************************************************
 *
 * Description : Left Pad
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort            sSourceCharCount;
    UShort            sPadCharCount;
    UShort            sCurCharCount;
    UShort            sResultIndex;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sPadIndex;
    UChar           * sPadFence;

    // To Fix BUG-12688
    // Source�� ���ڿ��� ����( = ���ڰ��� )�� ����
    sSourceCharCount = 0;
    sSourceIndex = aSource;
    sSourceFence = sSourceIndex + aSourceLen;

    while ( sSourceIndex < sSourceFence )
    {
        // TASK-3420 ���ڿ� ó�� ��å ����
        (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );

        ++sSourceCharCount;
    }

    /* BUG-19540                               *
     * ; validate the second arg. of Lpad(...) */
    if( aLength <= 0 )
    {
        aLength = 0;
        *aResultLen = (UShort) aLength;
    }
    else 
    {
        if( aLength <= sSourceCharCount )
        {
            //----------------------------------------------------------
            // Source�� ���� ������ �־��� ��� ���ڿ� ���̺��� ū ���,
            // ��� ���ڿ� ���̸�ŭ source���� result�� ���ڿ� copy
            //----------------------------------------------------------
        
            sSourceIndex = aSource;
            sCurCharCount = 0;

            while ( 1 )
            {
                // TASK-3420 ���ڿ� ó�� ��å ����
                (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );

                ++sCurCharCount;

                if ( sCurCharCount >= aLength )
                {
                    break;
                }
                else
                {
                    // nothing to do
                }
            }
        
            idlOS::memcpy( aResult,
                           aSource,
                           sSourceIndex - aSource );
            
            *aResultLen = sSourceIndex - aSource;
        }
        else
        {
            //----------------------------------------------------------
            // Source�� ���� ������ �־��� ��� ���ڿ� ���̺��� ���� ���,
            // ( ��� ���ڿ� - source ���ڿ� ) ��ŭ padding ���ڸ� ���ʿ�
            // ä�� ��, source ���ڿ��� ����� �����Ѵ�.
            //----------------------------------------------------------
        
            // Pad ���ڿ��� ����
            sPadCharCount = 0;

            sPadIndex = aPad;
            sPadFence = sPadIndex + aPadLen;

            while( sPadIndex < sPadFence )
            {
                // TASK-3420 ���ڿ� ó�� ��å ����
                (void)aLanguage->nextCharPtr( & sPadIndex, sPadFence );

                ++sPadCharCount;
            }

            // Pad ���ڿ��� ����� ����
            sPadIndex = 0;
            sResultIndex = 0;
            sCurCharCount = 0;

            if ( sPadCharCount < aLength - sSourceCharCount )
            {
                while ( 1 )
                {
                    idlOS::memcpy( aResult + sResultIndex,
                                   aPad,
                                   aPadLen );
                    
                    sResultIndex += aPadLen;
                    sCurCharCount += sPadCharCount;

                    if ( ( sCurCharCount + sPadCharCount ) >
                         ( aLength - sSourceCharCount ) )
                    {
                        // ���� Padding ������ pad ���ڿ����� ���� ���
                        break;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
            else
            {
                // nothing to do
            }

            // ���� padding ������ŭ pad ���� ����
            if ( sCurCharCount < aLength - sSourceCharCount )
            {
                sPadIndex = aPad;

                while ( sCurCharCount < aLength - sSourceCharCount )
                {
                    // TASK-3420 ���ڿ� ó�� ��å ����
                    (void)aLanguage->nextCharPtr( & sPadIndex, sPadFence );

                    ++sCurCharCount;
                }

                idlOS::memcpy( aResult + sResultIndex,
                               aPad,
                               sPadIndex - aPad );

                sResultIndex += sPadIndex - aPad;
            }
            else
            {
                // nothing to do
            }

            // Source ���ڿ��� ����� ����
            idlOS::memcpy( aResult + sResultIndex,
                           aSource,
                           aSourceLen );

            *aResultLen = sResultIndex + aSourceLen;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC mtfLpadCalculateFor2Args( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Lpad Calculate with 2 arguments
 *
 * Implementation :
 *    LPAD( char1, n )
 *
 *    aStack[0] : ù��° ���ڿ� char1�� ���ʺ��� ' '�� �����Ͽ�
 *                ������ ���� n��ŭ�� ' '�� ���Ե� char1 ��
 *    aStack[1] : char1 ( ���ڿ� )
 *    aStack[2] : n ( ��� ���ڿ� ���� )
 *
 *    ex) LPAD( 'ab', 5 ) ==> '   ab' 
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdIntegerType    sLength;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sLength   = *(mtdIntegerType*)aStack[2].value;
        
        // BUG-25914 : lpad�� ���̰� �������� precision�� ���� �� ����.
        IDE_TEST_RAISE( (sLength > 0) &&
                        (aStack[0].column->precision < sLength),
                        ERR_INVALID_LENGTH );

        // ------------------------------
        // LeftPad ���� �Լ�
        // ------------------------------

        IDE_TEST( mtfLeftPad( aStack[1].column->language,
                              sResult->value,
                              &sResult->length,
                              sSource->value,
                              sSource->length,
                              sLength,
                              (UChar *)" ",
                              1 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLpadCalculateFor3Args( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Lpad Calculate with 3 arguments
 *
 * Implementation :
 *    LPAD( char1, n, char2 )
 *
 *    aStack[0] : ù��° ���ڿ� char1�� ���ʺ��� char2�� �����Ͽ�
 *                ������ ���� n��ŭ�� char2�� ���Ե� char1 ��
 *    aStack[1] : char1 ( ���ڿ� )
 *    aStack[2] : n ( ��� ���ڿ� ���� )
 *    aStack[3] : char2 ( Left�� Pad �� ���ڿ� )
 *
 *    ex) LPAD( 'ab', 5, xy ) ==> 'xyxab' 
 *
 ***********************************************************************/

    mtdCharType*      sResult;
    mtdCharType*      sSource;
    mtdIntegerType    sLength;
    mtdCharType*      sPad;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sLength   = *(mtdIntegerType*)aStack[2].value;
        sPad      = (mtdCharType*)aStack[3].value;

        // BUG-25914 : lpad�� ���̰� �������� precision�� ���� �� ����.
        IDE_TEST_RAISE( (sLength > 0) &&
                        (aStack[0].column->precision < sLength),
                        ERR_INVALID_LENGTH );

        // ------------------------------
        // LeftPad ���� �Լ�
        // ------------------------------

        IDE_TEST( mtfLeftPad( aStack[1].column->language,
                              sResult->value,
                              &sResult->length,
                              sSource->value,
                              sSource->length,
                              sLength,
                              sPad->value,
                              sPad->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
            
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLpadCalculateNcharFor2Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Lpad Calculate with 2 arguments for NCHAR
 *
 * Implementation :
 *    LPAD( char1, n )
 *
 *    aStack[0] : ù��° ���ڿ� char1�� ���ʺ��� ' '�� �����Ͽ�
 *                ������ ���� n��ŭ�� ' '�� ���Ե� char1 ��
 *    aStack[1] : char1 ( ���ڿ� )
 *    aStack[2] : n ( ��� ���ڿ� ���� )
 *
 *    ex) LPAD( 'ab', 5 ) ==> '   ab' 
 *
 ***********************************************************************/
    
    const mtlModule  * sLanguage;
    mtdNcharType     * sResult;
    mtdNcharType     * sSource;
    mtdIntegerType     sLength;
    UShort             sPadLen;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sLanguage = aStack[1].column->language;
        sResult   = (mtdNcharType*)aStack[0].value;
        sSource   = (mtdNcharType*)aStack[1].value;
        sLength   = *(mtdIntegerType*)aStack[2].value;

        IDE_TEST_RAISE( aStack[0].column->precision < sLength,
                        ERR_INVALID_LENGTH );

        // ------------------------------
        // LeftPad ���� �Լ�
        // ------------------------------

        if( sLanguage->id == MTL_UTF16_ID )
        {
            sPadLen = MTL_UTF16_PRECISION;

            IDE_TEST( mtfLeftPad( sLanguage,
                                  sResult->value,
                                  &sResult->length,
                                  sSource->value,
                                  sSource->length,
                                  sLength,
                                  (UChar *)sLanguage->specialCharSet[MTL_SP_IDX],
                                  sPadLen )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( mtfLeftPad( sLanguage,
                                  sResult->value,
                                  &sResult->length,
                                  sSource->value,
                                  sSource->length,
                                  sLength,
                                  (UChar *)" ",
                                  1 )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLpadCalculateNcharFor3Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Lpad Calculate with 3 arguments for NCHAR
 *
 * Implementation :
 *    LPAD( char1, n, char2 )
 *
 *    aStack[0] : ù��° ���ڿ� char1�� ���ʺ��� char2�� �����Ͽ�
 *                ������ ���� n��ŭ�� char2�� ���Ե� char1 ��
 *    aStack[1] : char1 ( ���ڿ� )
 *    aStack[2] : n ( ��� ���ڿ� ���� )
 *    aStack[3] : char2 ( Left�� Pad �� ���ڿ� )
 *
 *    ex) LPAD( 'ab', 5, xy ) ==> 'xyxab' 
 *
 ***********************************************************************/

    mtdNcharType     * sResult;
    mtdNcharType     * sSource;
    mtdNcharType     * sPad;
    mtdIntegerType     sLength;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdNcharType*)aStack[0].value;
        sSource   = (mtdNcharType*)aStack[1].value;
        sLength   = *(mtdIntegerType*)aStack[2].value;
        sPad      = (mtdNcharType*)aStack[3].value;

        IDE_TEST_RAISE( aStack[0].column->precision < sLength,
                        ERR_INVALID_LENGTH );

        // ------------------------------
        // LeftPad ���� �Լ�
        // ------------------------------

        IDE_TEST( mtfLeftPad( aStack[1].column->language,
                              sResult->value,
                              &sResult->length,
                              sSource->value,
                              sSource->length,
                              sLength,
                              sPad->value,
                              sPad->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}