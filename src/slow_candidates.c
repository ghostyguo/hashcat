/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "rp.h"
#include "rp_cpu.h"
#include "rp_kernel_on_cpu.h"
#include "rp_kernel_on_cpu_optimized.h"
#include "wordlist.h"
#include "mpsp.h"
#include "filehandling.h"
#include "slow_candidates.h"

void slow_candidates_seek (hashcat_ctx_t *hashcat_ctx, void *extra_info, const u64 cur, const u64 end)
{
  combinator_ctx_t     *combinator_ctx     = hashcat_ctx->combinator_ctx;
  straight_ctx_t       *straight_ctx       = hashcat_ctx->straight_ctx;
  user_options_t       *user_options       = hashcat_ctx->user_options;
  user_options_extra_t *user_options_extra = hashcat_ctx->user_options_extra;

  const u32 attack_mode = user_options->attack_mode;

  if (attack_mode == ATTACK_MODE_STRAIGHT)
  {
    extra_info_straight_t *extra_info_straight = (extra_info_straight_t *) extra_info;

    FILE *fp = extra_info_straight->fp;

    const u64 overlap = end % straight_ctx->kernel_rules_cnt;

    const u64 init = end - overlap;

    for (u64 i = cur; i < init; i += straight_ctx->kernel_rules_cnt)
    {
      char *line_buf;
      u32   line_len;

      while (1)
      {
        get_next_word (hashcat_ctx, fp, &line_buf, &line_len);

        // post-process rule engine

        char rule_buf_out[RP_PASSWORD_SIZE];

        if (run_rule_engine ((int) user_options_extra->rule_len_l, user_options->rule_buf_l))
        {
          if (line_len >= RP_PASSWORD_SIZE) continue;

          memset (rule_buf_out, 0, sizeof (rule_buf_out));

          const int rule_len_out = _old_apply_rule (user_options->rule_buf_l, (int) user_options_extra->rule_len_l, line_buf, (int) line_len, rule_buf_out);

          if (rule_len_out < 0) continue;

          line_buf = rule_buf_out;
          line_len = (u32) rule_len_out;
        }

        break;
      }

      memcpy (extra_info_straight->base_buf, line_buf, line_len);

      extra_info_straight->base_len = line_len;
    }

    extra_info_straight->rule_pos_prev = overlap;
    extra_info_straight->rule_pos      = overlap;
  }
  else if (attack_mode == ATTACK_MODE_COMBI)
  {
    extra_info_combi_t *extra_info_combi = (extra_info_combi_t *) extra_info;

    FILE *base_fp  = extra_info_combi->base_fp;
    FILE *combs_fp = extra_info_combi->combs_fp;

          u64 overlap_cur = cur % combinator_ctx->combs_cnt;
    const u64 overlap     = end % combinator_ctx->combs_cnt;

    const u64 init = end - overlap;

    for (u64 i = cur; i < init; i += combinator_ctx->combs_cnt)
    {
      char *line_buf = NULL;
      u32   line_len = 0;

      while (1)
      {
        get_next_word (hashcat_ctx, base_fp, &line_buf, &line_len);

        // post-process rule engine

        if (run_rule_engine ((int) user_options_extra->rule_len_l, user_options->rule_buf_l))
        {
          if (line_len >= RP_PASSWORD_SIZE) continue;

          char rule_buf_out[RP_PASSWORD_SIZE];

          memset (rule_buf_out, 0, sizeof (rule_buf_out));

          const int rule_len_out = _old_apply_rule (user_options->rule_buf_l, (int) user_options_extra->rule_len_l, line_buf, (int) line_len, rule_buf_out);

          if (rule_len_out < 0) continue;
        }

        break;
      }

      memcpy (extra_info_combi->base_buf, line_buf, line_len);

      extra_info_combi->base_len = line_len;

      rewind (combs_fp);

      overlap_cur = 0;
    }

    for (u64 i = overlap_cur; i < overlap; i++)
    {
      char *line_buf = extra_info_combi->scratch_buf;
      u32   line_len = 0;

      while (1)
      {
        line_len = (u32) fgetl (combs_fp, line_buf);

        // post-process rule engine

        if (run_rule_engine ((int) user_options_extra->rule_len_l, user_options->rule_buf_l))
        {
          if (line_len >= RP_PASSWORD_SIZE) continue;

          char rule_buf_out[RP_PASSWORD_SIZE];

          memset (rule_buf_out, 0, sizeof (rule_buf_out));

          const int rule_len_out = _old_apply_rule (user_options->rule_buf_l, (int) user_options_extra->rule_len_l, line_buf, (int) line_len, rule_buf_out);

          if (rule_len_out < 0) continue;
        }

        break;
      }
    }

    extra_info_combi->comb_pos_prev = overlap;
    extra_info_combi->comb_pos      = overlap;
  }
  else if (attack_mode == ATTACK_MODE_BF)
  {
    // nothing to do
  }
}

void slow_candidates_next (hashcat_ctx_t *hashcat_ctx, void *extra_info)
{
  hashconfig_t         *hashconfig         = hashcat_ctx->hashconfig;
  combinator_ctx_t     *combinator_ctx     = hashcat_ctx->combinator_ctx;
  mask_ctx_t           *mask_ctx           = hashcat_ctx->mask_ctx;
  straight_ctx_t       *straight_ctx       = hashcat_ctx->straight_ctx;
  user_options_t       *user_options       = hashcat_ctx->user_options;
  user_options_extra_t *user_options_extra = hashcat_ctx->user_options_extra;

  const u32 attack_mode = user_options->attack_mode;

  if (attack_mode == ATTACK_MODE_STRAIGHT)
  {
    extra_info_straight_t *extra_info_straight = (extra_info_straight_t *) extra_info;

    FILE *fp = extra_info_straight->fp;

    if (extra_info_straight->rule_pos == 0)
    {
      char *line_buf;
      u32   line_len;

      while (1)
      {
        get_next_word (hashcat_ctx, fp, &line_buf, &line_len);

        line_len = (u32) convert_from_hex (hashcat_ctx, line_buf, line_len);

        // post-process rule engine

        char rule_buf_out[RP_PASSWORD_SIZE];

        if (run_rule_engine ((int) user_options_extra->rule_len_l, user_options->rule_buf_l))
        {
          if (line_len >= RP_PASSWORD_SIZE) continue;

          memset (rule_buf_out, 0, sizeof (rule_buf_out));

          const int rule_len_out = _old_apply_rule (user_options->rule_buf_l, (int) user_options_extra->rule_len_l, line_buf, (int) line_len, rule_buf_out);

          if (rule_len_out < 0) continue;

          line_buf = rule_buf_out;
          line_len = (u32) rule_len_out;
        }

        break;
      }

      memcpy (extra_info_straight->base_buf, line_buf, line_len);

      extra_info_straight->base_len = line_len;
    }

    memcpy (extra_info_straight->out_buf, extra_info_straight->base_buf, extra_info_straight->base_len);

    extra_info_straight->out_len = extra_info_straight->base_len;

    memset (extra_info_straight->out_buf + extra_info_straight->base_len, 0, sizeof (extra_info_straight->out_buf) - extra_info_straight->out_len);

    u32 *out_ptr = (u32 *) extra_info_straight->out_buf;

    if (hashconfig->opti_type & OPTI_TYPE_OPTIMIZED_KERNEL)
    {
      extra_info_straight->out_len = apply_rules_optimized (straight_ctx->kernel_rules_buf[extra_info_straight->rule_pos].cmds, &out_ptr[0], &out_ptr[4], extra_info_straight->out_len);
    }
    else
    {
      extra_info_straight->out_len = apply_rules (straight_ctx->kernel_rules_buf[extra_info_straight->rule_pos].cmds, out_ptr, extra_info_straight->out_len);
    }

    extra_info_straight->rule_pos_prev = extra_info_straight->rule_pos;

    extra_info_straight->rule_pos++;

    if (extra_info_straight->rule_pos == straight_ctx->kernel_rules_cnt)
    {
      extra_info_straight->rule_pos = 0;
    }
  }
  else if (attack_mode == ATTACK_MODE_COMBI)
  {
    extra_info_combi_t *extra_info_combi = (extra_info_combi_t *) extra_info;

    FILE *base_fp  = extra_info_combi->base_fp;
    FILE *combs_fp = extra_info_combi->combs_fp;

    if (extra_info_combi->comb_pos == 0)
    {
      char *line_buf;
      u32   line_len;

      while (1)
      {
        get_next_word (hashcat_ctx, base_fp, &line_buf, &line_len);

        line_len = (u32) convert_from_hex (hashcat_ctx, line_buf, line_len);

        // post-process rule engine

        if (run_rule_engine ((int) user_options_extra->rule_len_l, user_options->rule_buf_l))
        {
          if (line_len >= RP_PASSWORD_SIZE) continue;

          char rule_buf_out[RP_PASSWORD_SIZE];

          memset (rule_buf_out, 0, sizeof (rule_buf_out));

          const int rule_len_out = _old_apply_rule (user_options->rule_buf_l, (int) user_options_extra->rule_len_l, line_buf, (int) line_len, rule_buf_out);

          if (rule_len_out < 0) continue;
        }

        break;
      }

      memcpy (extra_info_combi->base_buf, line_buf, line_len);

      extra_info_combi->base_len = line_len;

      rewind (combs_fp);
    }

    memcpy (extra_info_combi->out_buf, extra_info_combi->base_buf, extra_info_combi->base_len);

    extra_info_combi->out_len = extra_info_combi->base_len;

    char *line_buf = extra_info_combi->scratch_buf;

    u32 line_len = 0;

    while (1)
    {
      line_len = (u32) fgetl (combs_fp, line_buf);

      // post-process rule engine

      if (run_rule_engine ((int) user_options_extra->rule_len_r, user_options->rule_buf_r))
      {
        if (line_len >= RP_PASSWORD_SIZE) continue;

        char rule_buf_out[RP_PASSWORD_SIZE];

        memset (rule_buf_out, 0, sizeof (rule_buf_out));

        const int rule_len_out = _old_apply_rule (user_options->rule_buf_r, (int) user_options_extra->rule_len_r, line_buf, (int) line_len, rule_buf_out);

        if (rule_len_out < 0) continue;
      }

      break;
    }

    memcpy (extra_info_combi->out_buf + extra_info_combi->out_len, line_buf, line_len);

    extra_info_combi->out_len += line_len;

    memset (extra_info_combi->out_buf + extra_info_combi->out_len, 0, sizeof (extra_info_combi->out_buf) - extra_info_combi->out_len);

    extra_info_combi->comb_pos_prev = extra_info_combi->comb_pos;

    extra_info_combi->comb_pos++;

    if (extra_info_combi->comb_pos == combinator_ctx->combs_cnt)
    {
      extra_info_combi->comb_pos = 0;
    }
  }
  else if (attack_mode == ATTACK_MODE_BF)
  {
    extra_info_mask_t *extra_info_mask = (extra_info_mask_t *) extra_info;

    sp_exec (extra_info_mask->pos, (char *) extra_info_mask->out_buf, mask_ctx->root_css_buf, mask_ctx->markov_css_buf, 0, mask_ctx->css_cnt);
  }
}
