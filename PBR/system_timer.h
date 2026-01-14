/*==============================================================================

   �V�X�e���^�C�}�[ [system_timer.h]
                                                         Author : Youhei Sato
                                                         Date   : 2018/06/17
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SYSTEM_TIMER_H_
#define SYSTEM_TIMER_H_


/*------------------------------------------------------------------------------
   �֐��̃v���g�^�C�v�錾
------------------------------------------------------------------------------*/

// �V�X�e���^�C�}�[�̏�����
void SystemTimer_Initialize(void);

// �V�X�e���^�C�}�[�̃��Z�b�g
void SystemTimer_Reset(void);

// �V�X�e���^�C�}�[�̃X�^�[�g
void SystemTimer_Start(void);

// �V�X�e���^�C�}�[�̃X�g�b�v
void SystemTimer_Stop(void);

// �V�X�e���^�C�}�[��0.1�b�i�߂�
void SystemTimer_Advance(void);

// �v�����Ԃ̎擾
double SystemTimer_GetTime(void);

// ���݂̎��Ԃ�擾
double SystemTimer_GetAbsoluteTime(void);

// �o�ߎ��Ԃ̎擾
double SystemTimer_GetElapsedTime(void);

// �V�X�e���^�C�}�[���~�܂��Ă��邩�H
bool SystemTimer_IsStoped(void);

// ���݂̃X���b�h��1�̃v���Z�b�T�i���݂̃X���b�h�j�ɐ���
void LimitThreadAffinityToCurrentProc(void);

#endif // SYSTEM_TIMER_H_
