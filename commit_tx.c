#include "commit_tx.h"
#include "shadouble.h"
#include "bitcoin_tx.h"
#include "bitcoin_script.h"
#include "permute_tx.h"

struct bitcoin_tx *create_commit_tx(const tal_t *ctx,
				    OpenChannel *ours,
				    OpenChannel *theirs,
				    const struct sha256_double *anchor_txid,
				    unsigned int anchor_output)
{
	struct bitcoin_tx *tx;
	const u8 *redeemscript;

	/* Now create commitment tx: one input, two outputs. */
	tx = bitcoin_tx(ctx, 1, 2);

	/* Our input spends the anchor tx output. */
	tx->input[0].txid = *anchor_txid;
	tx->input[0].index = anchor_output;

	/* First output is a P2SH to a complex redeem script (usu. for me) */
	redeemscript = bitcoin_redeem_revocable(tx, ours->anchor->pubkey,
						ours->locktime_seconds,
						theirs->anchor->pubkey,
						ours->revocation_hash);
	tx->output[0].script = scriptpubkey_p2sh(tx, redeemscript);
	tx->output[0].script_length = tal_count(tx->output[0].script);

	if (ours->anchor->total < ours->commitment_fee)
		return tal_free(tx);
	tx->output[0].amount = ours->anchor->total - ours->commitment_fee;

	/* Second output is a simple payment to them. */
	tx->output[1].script = theirs->script_to_me.data;
	tx->output[1].script_length = theirs->script_to_me.len;
	
	if (theirs->anchor->total < theirs->commitment_fee)
		return tal_free(tx);
	tx->output[1].amount = theirs->anchor->total - theirs->commitment_fee;

	permute_outputs(ours->seed, theirs->seed, 1, tx->output, 2, NULL);
	return tx;
}