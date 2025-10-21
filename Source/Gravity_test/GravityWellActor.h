#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityWellActor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class ACharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogGravityWell, Log, All);

struct FAffectedCharacterState
{
    TWeakObjectPtr<ACharacter> Character;
    float PreviousGravityScale = 1.f;
    uint8 PreviousMovementMode = 0;
};

/**
 * Simple gravity well actor that attracts overlapping physics objects and characters.
 */
UCLASS(Blueprintable)
class GRAVITY_TEST_API AGravityWellActor : public AActor
{
    GENERATED_BODY()

public:
    AGravityWellActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> InfluenceSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> VisualizationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraComponent> AccretionVfxComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell", meta = (ClampMin = "0.0"))
    float Strength = 3000000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell", meta = (ClampMin = "0.0"))
    float MaxRadius = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell", meta = (ClampMin = "1.0"))
    float MinRadius = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell", meta = (ClampMin = "0.0"))
    float MaxAccel = 6000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell")
    bool bAffectRigidBodies = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell")
    bool bAffectCharacters = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GravityWell", meta = (ClampMin = "0.005"))
    float TickInterval = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization")
    bool bEnableVisualization = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization"))
    TObjectPtr<UMaterialInterface> VisualizationMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization"))
    TObjectPtr<UNiagaraSystem> AccretionNiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization", ClampMin = "1.0"))
    float VisualizationMeshReferenceRadius = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization", ClampMin = "0.0"))
    float PulseSpeed = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization", ClampMin = "0.0"))
    float PulseIntensity = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization"))
    FName RadiusParameterName = TEXT("InfluenceRadius");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization"))
    FName StrengthParameterName = TEXT("GravityStrength");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization", meta = (EditCondition = "bEnableVisualization"))
    FName PulseParameterName = TEXT("PulsePhase");

    virtual FVector ComputeAcceleration(const FVector& WellLocation, const FVector& TargetLocation) const;

private:
    void UpdateSphereRadius();
    void StartGravityTimer();
    void ApplyGravityTick();
    void RestoreCharacterGravity(TWeakObjectPtr<ACharacter> CharacterPtr);
    void RestoreAllCharacters();
    FAffectedCharacterState* FindCharacterState(const TWeakObjectPtr<ACharacter>& CharacterPtr);
    void RemoveCharacterState(const TWeakObjectPtr<ACharacter>& CharacterPtr);

    void RefreshVisualizationAssets();
    void UpdateVisualizationActivation();
    void UpdateVisualizationScale();
    void UpdateVisualizationParameters(float DeltaSeconds);

    FTimerHandle GravityTimerHandle;
    TSet<TWeakObjectPtr<ACharacter>> AffectedCharacters;
    TArray<FAffectedCharacterState> CharacterStates;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> VisualizationMID;

    float PulseAccumulator = 0.f;
};
