/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "blapi.h"
#include "ec.h"
#include "ecl-curve.h"
#include "prprf.h"
#include "basicutil.h"
#include "secder.h"
#include "secitem.h"
#include "nspr.h"
#include <stdio.h>

typedef struct {
    ECCurveName curve;
    int iterations;
    char *privhex;
    char *our_pubhex;
    char *their_pubhex;
    char *common_key;
    char *name;
    ECFieldType fieldType;
} ECDH_KAT;

typedef struct {
    ECCurveName curve;
    char *point;
    char *name;
    ECFieldType fieldType;
} ECDH_BAD;

#include "testvecs.h"

/*
 * Initializes a SECItem from a hexadecimal string
 *
 */
static SECItem *
hexString2SECItem(PLArenaPool *arena, SECItem *item, const char *str)
{
    int i = 0;
    int byteval = 0;
    int tmp = PORT_Strlen(str);

    PORT_Assert(arena);
    PORT_Assert(item);

    if ((tmp % 2) != 0) {
        return NULL;
    }

    item = SECITEM_AllocItem(arena, item, tmp / 2);
    if (item == NULL) {
        return NULL;
    }

    while (str[i]) {
        if ((str[i] >= '0') && (str[i] <= '9')) {
            tmp = str[i] - '0';
        } else if ((str[i] >= 'a') && (str[i] <= 'f')) {
            tmp = str[i] - 'a' + 10;
        } else if ((str[i] >= 'A') && (str[i] <= 'F')) {
            tmp = str[i] - 'A' + 10;
        } else {
            /* item is in arena and gets freed by the caller */
            return NULL;
        }

        byteval = byteval * 16 + tmp;
        if ((i % 2) != 0) {
            item->data[i / 2] = byteval;
            byteval = 0;
        }
        i++;
    }

    return item;
}

void
printBuf(const SECItem *item)
{
    int i;
    if (!item || !item->len) {
        printf("(null)\n");
        return;
    }

    for (i = 0; i < item->len; i++) {
        printf("%02x", item->data[i]);
    }
    printf("\n");
}

/* Initialise test with basic curve populate with only the necessary things */
SECStatus
init_params(ECParams *ecParams, ECCurveName curve, PLArenaPool **arena,
            ECFieldType type)
{
    if ((curve < ECCurve_noName) || (curve > ECCurve_pastLastCurve)) {
        return SECFailure;
    }
    *arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!*arena) {
        return SECFailure;
    }
    ecParams->name = curve;
    ecParams->type = ec_params_named;
    ecParams->curveOID.data = NULL;
    ecParams->curveOID.len = 0;
    ecParams->curve.seed.data = NULL;
    ecParams->curve.seed.len = 0;
    ecParams->DEREncoding.data = NULL;
    ecParams->DEREncoding.len = 0;
    ecParams->arena = *arena;
    ecParams->fieldID.size = ecCurve_map[curve]->size;
    ecParams->fieldID.type = type;
    ecParams->cofactor = ecCurve_map[curve]->cofactor;
    ecParams->pointSize = ecCurve_map[curve]->pointSize;

    return SECSuccess;
}

SECStatus
ectest_ecdh_kat(ECDH_KAT *kat)
{
    ECCurveName curve = kat->curve;
    ECParams ecParams = { 0 };
    ECPrivateKey *ecPriv = NULL;
    SECItem theirKey = { siBuffer, NULL, 0 };
    SECStatus rv = SECFailure;
    PLArenaPool *arena = NULL;
    SECItem seed = { siBuffer, NULL, 0 };
    SECItem answer = { siBuffer, NULL, 0 };
    SECItem answer2 = { siBuffer, NULL, 0 };
    SECItem derived = { siBuffer, NULL, 0 };
    char genenc[3 + 2 * 2 * MAX_ECKEY_LEN];
    int i;

    rv = init_params(&ecParams, curve, &arena, kat->fieldType);
    if (rv != SECSuccess) {
        return rv;
    }

    hexString2SECItem(arena, &ecParams.fieldID.u.prime, ecCurve_map[curve]->irr);
    hexString2SECItem(arena, &ecParams.curve.a, ecCurve_map[curve]->curvea);
    hexString2SECItem(arena, &ecParams.curve.b, ecCurve_map[curve]->curveb);
    genenc[0] = '0';
    genenc[1] = '4';
    genenc[2] = '\0';
    PORT_Assert(PR_ARRAY_SIZE(genenc) >= PORT_Strlen(ecCurve_map[curve]->genx));
    PORT_Assert(PR_ARRAY_SIZE(genenc) >= PORT_Strlen(ecCurve_map[curve]->geny));
    strcat(genenc, ecCurve_map[curve]->genx);
    strcat(genenc, ecCurve_map[curve]->geny);
    hexString2SECItem(arena, &ecParams.base, genenc);
    hexString2SECItem(arena, &ecParams.order, ecCurve_map[curve]->order);

    if (kat->our_pubhex) {
        hexString2SECItem(arena, &answer, kat->our_pubhex);
    }
    hexString2SECItem(arena, &seed, kat->privhex);
    rv = EC_NewKeyFromSeed(&ecParams, &ecPriv, seed.data, seed.len);
    if (rv != SECSuccess) {
        rv = SECFailure;
        goto cleanup;
    }
    if (kat->our_pubhex) {
        if (SECITEM_CompareItem(&answer, &ecPriv->publicValue) != SECEqual) {
            rv = SECFailure;
            goto cleanup;
        }
    }

    hexString2SECItem(arena, &theirKey, kat->their_pubhex);
    hexString2SECItem(arena, &answer2, kat->common_key);

    rv = EC_ValidatePublicKey(&ecParams, &theirKey);
    if (rv != SECSuccess) {
        printf("EC_ValidatePublicKey failed\n");
        goto cleanup;
    }

    for (i = 0; i < kat->iterations; ++i) {
        rv = ECDH_Derive(&theirKey, &ecParams, &ecPriv->privateValue, PR_TRUE, &derived);
        if (rv != SECSuccess) {
            rv = SECFailure;
            goto cleanup;
        }
        rv = SECITEM_CopyItem(ecParams.arena, &theirKey, &ecPriv->privateValue);
        if (rv != SECSuccess) {
            goto cleanup;
        }
        rv = SECITEM_CopyItem(ecParams.arena, &ecPriv->privateValue, &derived);
        if (rv != SECSuccess) {
            goto cleanup;
        }
        SECITEM_FreeItem(&derived, PR_FALSE);
    }

    if (SECITEM_CompareItem(&answer2, &ecPriv->privateValue) != SECEqual) {
        printf("expected: ");
        printBuf(&answer2);
        printf("derived:  ");
        printBuf(&ecPriv->privateValue);
        rv = SECFailure;
        goto cleanup;
    }

cleanup:
    PORT_FreeArena(arena, PR_FALSE);
    if (ecPriv) {
        PORT_FreeArena(ecPriv->ecParams.arena, PR_FALSE);
    }
    if (derived.data) {
        SECITEM_FreeItem(&derived, PR_FALSE);
    }
    return rv;
}

SECStatus
ectest_validate_point(ECDH_BAD *bad)
{
    ECParams ecParams = { 0 };
    SECItem point = { siBuffer, NULL, 0 };
    SECStatus rv = SECFailure;
    PLArenaPool *arena = NULL;

    rv = init_params(&ecParams, bad->curve, &arena, bad->fieldType);
    if (rv != SECSuccess) {
        return rv;
    }

    hexString2SECItem(arena, &point, bad->point);
    rv = EC_ValidatePublicKey(&ecParams, &point);

    PORT_FreeArena(arena, PR_FALSE);
    return rv;
}

void
printUsage(char *prog)
{
    printf("Usage: %s [-fp] [-nd]\n"
           "\t-n: NIST curves\n"
           "\t-d: non-NIST curves\n"
           "You have to specify at at least one of n or d.\n"
           "By default no tests are executed.\n",
           prog);
}

/* Performs tests of elliptic curve cryptography over prime fields If
 * tests fail, then it prints an error message, aborts, and returns an
 * error code. Otherwise, returns 0. */
int
main(int argv, char **argc)
{
    SECStatus rv = SECSuccess;
    int numkats = 0;
    int i = 0;
    int nist = 0;
    int nonnist = 0;

    for (i = 1; i < argv; i++) {
        if (PL_strcasecmp(argc[i], "-n") == 0) {
            nist = 1;
        } else if (PL_strcasecmp(argc[i], "-d") == 0) {
            nonnist = 1;
        } else {
            printUsage(argc[0]);
            return 1;
        }
    }
    if (!nist && !nonnist) {
        printUsage(argc[0]);
        return 1;
    }

    rv = SECOID_Init();
    if (rv != SECSuccess) {
        SECU_PrintError("Error:", "SECOID_Init");
        goto cleanup;
    }

    /* Test P256, P384, P521 */
    if (nist) {
        while (ecdh_testvecs[numkats].curve != ECCurve_pastLastCurve) {
            numkats++;
        }
        printf("1..%d\n", numkats);
        for (i = 0; ecdh_testvecs[i].curve != ECCurve_pastLastCurve; i++) {
            if (ectest_ecdh_kat(&ecdh_testvecs[i]) != SECSuccess) {
                printf("not okay %d - %s\n", i + 1, ecdh_testvecs[i].name);
                rv = SECFailure;
            } else {
                printf("okay %d - %s\n", i + 1, ecdh_testvecs[i].name);
            }
        }
    }

    /* Test KAT for non-NIST curves */
    if (nonnist) {
        for (i = 0; nonnist_testvecs[i].curve != ECCurve_pastLastCurve; i++) {
            if (ectest_ecdh_kat(&nonnist_testvecs[i]) != SECSuccess) {
                printf("not okay %d - %s\n", i + 1, nonnist_testvecs[i].name);
                rv = SECFailure;
            } else {
                printf("okay %d - %s\n", i + 1, nonnist_testvecs[i].name);
            }
        }
        for (i = 0; nonnist_testvecs_bad_values[i].curve != ECCurve_pastLastCurve; i++) {
            if (ectest_validate_point(&nonnist_testvecs_bad_values[i]) == SECSuccess) {
                printf("not okay %d - %s\n", i + 1, nonnist_testvecs_bad_values[i].name);
                rv = SECFailure;
            } else {
                printf("okay %d - %s\n", i + 1, nonnist_testvecs_bad_values[i].name);
            }
        }
    }

cleanup:
    rv |= SECOID_Shutdown();

    if (rv != SECSuccess) {
        printf("Error: exiting with error value\n");
    }
    return rv;
}
